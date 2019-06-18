#!/usr/bin/env bash
#
# Copyright (c) 2018 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.

export LC_ALL=C.UTF-8

TRAVIS_COMMIT_LOG=$(git log --format=fuller -1)
export TRAVIS_COMMIT_LOG

OUTDIR=$BASE_OUTDIR/$TRAVIS_PULL_REQUEST/$TRAVIS_JOB_NUMBER-$HOST
BITCOIN_CONFIG_ALL="--disable-dependency-tracking --prefix=$TRAVIS_BUILD_DIR/depends/$HOST --bindir=$OUTDIR/bin --libdir=$OUTDIR/lib"
if [ -z "$NO_DEPENDS" ]; then
  DOCKER_EXEC ccache --max-size=$CCACHE_SIZE
fi

BEGIN_FOLD autogen
if [ -n "$CONFIG_SHELL" ]; then
  DOCKER_EXEC "$CONFIG_SHELL" -c "./autogen.sh"
else
  DOCKER_EXEC ./autogen.sh
fi
END_FOLD

mkdir build
cd build || (echo "could not enter build directory"; exit 1)

BEGIN_FOLD configure
DOCKER_EXEC ../configure --cache-file=config.cache $BITCOIN_CONFIG_ALL $BITCOIN_CONFIG || ( cat config.log && false)
END_FOLD

#BEGIN_FOLD distdir
#DOCKER_EXEC make distdir VERSION=$HOST
#END_FOLD

#cd "ucc-$HOST" || (echo "could not enter distdir ucc-$HOST"; exit 1)

#if [ "$BUILD_ONLY_DEPENDS" = "false" ]; then
#BEGIN_FOLD configure
#DOCKER_EXEC ./configure --cache-file=../config.cache $BITCOIN_CONFIG_ALL $BITCOIN_CONFIG || ( cat config.log && false)
#END_FOLD
#fi

if [ "$BUILD_ONLY_DEPENDS" = "false" ]; then
BEGIN_FOLD build
DOCKER_EXEC make $MAKEJOBS $GOAL || ( echo "Build failure. Verbose build follows." && DOCKER_EXEC make $GOAL V=1 ; false )
END_FOLD
fi

if [ "$RUN_UNIT_TESTS" = "true" ]; then
  BEGIN_FOLD unit-tests
  DOCKER_EXEC LD_LIBRARY_PATH=$TRAVIS_BUILD_DIR/depends/$HOST/lib make $MAKEJOBS check VERBOSE=1
  END_FOLD
fi

if [ "$RUN_BENCH" = "true" ]; then
  BEGIN_FOLD bench
  DOCKER_EXEC LD_LIBRARY_PATH=$TRAVIS_BUILD_DIR/depends/$HOST/lib $OUTDIR/bin/bench_ucc -scaling=0.001
  END_FOLD
fi

if [ "$TRAVIS_EVENT_TYPE" = "cron" ]; then
  extended="--extended --exclude feature_pruning"
fi

if [ "$RUN_FUNCTIONAL_TESTS" = "true" ]; then
  BEGIN_FOLD functional-tests
  DOCKER_EXEC test/functional/test_runner.py --combinedlogslen=4000 --coverage --quiet --failfast ${extended}
  END_FOLD
fi

#deploy test builds
if [ "$DEPLOY_TEST_BUILDS" = "true" ] && [ "$BUILD_ONLY_DEPENDS" = "false" ]; then
  BEGIN_FOLD deploytests
#  DOCKER_EXEC export VERSION="$REASON-$TRAVIS_BRANCH"
  if [ "$REASON" = "MacOS" ]; then DOCKER_EXEC pwd && find . -name '*.dmg' && echo $OUTDIR && mkdir -p "$TRAVIS_BUILD_DIR/release" && cp *.dmg "$TRAVIS_BUILD_DIR/release/UCC-$REASON-$TRAVIS_BRANCH.dmg" && cd "$TRAVIS_BUILD_DIR/release" && ls; fi
  if [ "$REASON" != "MacOS" ]; then DOCKER_EXEC pwd && mkdir -p "$TRAVIS_BUILD_DIR/release/UCC-$REASON-$TRAVIS_BRANCH" && find $OUTDIR && cd $OUTDIR && pwd && ls -R * && cp -a bin/* "$TRAVIS_BUILD_DIR/release/UCC-$REASON-$TRAVIS_BRANCH/" && cd "$TRAVIS_BUILD_DIR/release" && strip UCC-$REASON-$TRAVIS_BRANCH/* && ls UCC-$REASON-$TRAVIS_BRANCH/* && zip -r UCC-$REASON-$TRAVIS_BRANCH.zip * && ls; fi
  DOCKER_EXEC git init
  DOCKER_EXEC git config --global user.email "3713548+flyinghuman@users.noreply.github.com"
  DOCKER_EXEC git config --global user.name "Travis-User"
  DOCKER_EXEC git add --force --all
  DOCKER_EXEC git commit -m "Latest-Build"
  DOCKER_EXEC git remote add origin https://github.com/flyinghuman/ucc-builds.git
  DOCKER_EXEC git push -f -u https://$BUILDTOKEN@github.com/flyinghuman/ucc-builds.git master:$REASON
  END_FOLD
fi

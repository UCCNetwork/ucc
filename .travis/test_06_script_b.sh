#!/usr/bin/env bash
#
# Copyright (c) 2018 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.

export LC_ALL=C.UTF-8

#deploy test builds
if [ "$DEPLOY_TEST_BUILDS" = "true" ] && [ "$BUILD_ONLY_DEPENDS" = "false" ]; then
  BEGIN_FOLD deploybins
  if [ "$REASON" = "MacOS" ]; then DOCKER_EXEC pwd && find . -name '*.dmg' && echo $OUTDIR && mkdir -p "$TRAVIS_BUILD_DIR/release" && cp *.dmg "$TRAVIS_BUILD_DIR/release/UCC-$REASON-$TRAVIS_BRANCH.dmg" && cd "$TRAVIS_BUILD_DIR/release" && ls; fi
  if [ "$REASON" != "MacOS" ]; then DOCKER_EXEC pwd && mkdir -p "$TRAVIS_BUILD_DIR/release/UCC-$REASON-$TRAVIS_BRANCH" && find $OUTDIR && cd $OUTDIR && pwd && ls -R && cp -a bin/* "$TRAVIS_BUILD_DIR/release/UCC-$REASON-$TRAVIS_BRANCH/" && cd "$TRAVIS_BUILD_DIR/release" && strip UCC-$REASON-$TRAVIS_BRANCH/* && ls UCC-$REASON-$TRAVIS_BRANCH/* && zip -r UCC-$REASON-$TRAVIS_BRANCH.zip * && ls; fi
  DOCKER_EXEC git init
  DOCKER_EXEC git config --global user.email "3713548+flyinghuman@users.noreply.github.com"
  DOCKER_EXEC git config --global user.name "Travis-User"
  DOCKER_EXEC git add --force --all
  DOCKER_EXEC git commit -m "Latest-Build"
  DOCKER_EXEC git remote add origin https://github.com/flyinghuman/ucc-builds.git
  DOCKER_EXEC git push -f -u https://$BUILDTOKEN@github.com/flyinghuman/ucc-builds.git master:$REASON
  END_FOLD
fi

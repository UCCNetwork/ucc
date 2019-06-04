// Copyright (c) 2010 Satoshi Nakamoto
// Copyright (c) 2009-2014 The Bitcoin developers
// Copyright (c) 2014-2015 The Dash developers
// Copyright (c) 2015-2017 The PIVX developers
// Copyright (c) 2017-2018 The XDNA Core developers
// Copyright (c) 2018-2019 The UCC Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "chainparams.h"
#include "random.h"
#include "util.h"
#include "utilstrencodings.h"

#include <mutex>
#include <assert.h>
#include <limits>

#include <boost/assign/list_of.hpp>

using namespace std;
using namespace boost::assign;

struct SeedSpec6 {
    uint8_t addr[16];
    uint16_t port;
};

#include "chainparamsseeds.h"

/**
 * Main network
 */

//! Convert the pnSeeds6 array into usable address objects.
static void convertSeed6(std::vector<CAddress>& vSeedsOut, const SeedSpec6* data, unsigned int count)
{
    // It'll only connect to one or two seed nodes because once it connects,
    // it'll get a pile of addresses with newer timestamps.
    // Seed nodes are given a random 'last seen time' of between one and two
    // weeks ago.
    const int64_t nOneWeek = 7 * 24 * 60 * 60;
    for (unsigned int i = 0; i < count; i++) {
        struct in6_addr ip;
        memcpy(&ip, data[i].addr, sizeof(ip));
        CAddress addr(CService(ip, data[i].port));
        addr.nTime = GetTime() - GetRand(nOneWeek) - nOneWeek;
        vSeedsOut.push_back(addr);
    }
}

//   What makes a good checkpoint block?
// + Is surrounded by blocks with reasonable timestamps
//   (no blocks before with a timestamp after, none after with
//    timestamp before)
// + Contains no strange transactions
static Checkpoints::MapCheckpoints mapCheckpoints =
    boost::assign::map_list_of(0, uint256("0x000000c914b2c4a05d8e28f4ec8498b7f13798cd3513cf856c0da23690c00a89"))
                              (50, uint256("0x00000095487e7b6c6e10eca3d1c16f42af2b7daa85c6ebc3880647361aa22ba4"))
                              (60200, uint256("0x000000000b791c519d51f8547ccdb4ca6657e7ebc4bcd3a11a01ade338074066"))
							  (91268, uint256("0x00000000288b93b6501a6b877a029064b58963d6d892f275f23e04a5945ff64a"));

static const Checkpoints::CCheckpointData data = {
    &mapCheckpoints,
    1542267487, // * UNIX timestamp of last checkpoint block
    93975,          // * total number of transactions between genesis and last checkpoint
                //   (the tx=... number in the SetBestChain debug.log lines)
    2000        // * estimated number of transactions per day after checkpoint
};

static Checkpoints::MapCheckpoints mapCheckpointsTestnet =
    boost::assign::map_list_of(0, uint256("00000c5daaf65811373c50c5484f7453bef8e7ee071f275112a84f53b422369c"));
static const Checkpoints::CCheckpointData dataTestnet = {
    &mapCheckpointsTestnet,
    1536512401,
    0,
    250};

static Checkpoints::MapCheckpoints mapCheckpointsRegtest =
    boost::assign::map_list_of(0, uint256("0000065c21928f425a473b9e61b4f25865077176507d5b80ed4260bb4f8b1a89"));
static const Checkpoints::CCheckpointData dataRegtest = {
    &mapCheckpointsRegtest,
    1536512402,
    0,
    100};

CAmount CChainParams::SubsidyValue(SubsidySwitchPoints::key_type level) const
{
    const auto& points = subsidySwitchPoints;

    SubsidySwitchPoints::const_iterator point = points.upper_bound(level);

    if(point != subsidySwitchPoints.begin())
        point = std::prev(point);

    return point->second;
}

void MineGenesis(CBlock genesis)
{
    printf("Searching for genesis block...\n");
    // This will figure out a valid hash and Nonce if you're
    // creating a different genesis block:
    uint256 hashTarget = ~uint256(0) >> 20;
    uint256 thash;
    while(true)
    {
        thash = genesis.GetHash();
        if (thash <= hashTarget)
            break;
        if ((genesis.nNonce & 0xFFF) == 0)
        {
            printf("nonce %u: hash = %s \n", genesis.nNonce, thash.ToString().c_str());
        }
        ++genesis.nNonce;
        if (genesis.nNonce == 0)
        {
            printf("NONCE WRAPPED, incrementing time\n");
            ++genesis.nTime;
        }
    }
    printf("block.nTime = %u \n", genesis.nTime);
    printf("block.nNonce = %u \n", genesis.nNonce);
    printf("block.GetHash = %s\n", genesis.GetHash().ToString().c_str());
    printf("block.merkle = %s\n", genesis.hashMerkleRoot.ToString().c_str());
    std::fflush(stdout);
}

class CMainParams : public CChainParams
{
public:
    CMainParams()
    {
        networkID = CBaseChainParams::MAIN;
        strNetworkID = "main";
        /**
         * The message start string is designed to be unlikely to occur in normal data.
         * The characters are rarely used upper ASCII, not valid as UTF-8, and produce
         * a large 4-byte int at any alignment.
         */
        pchMessageStart[0] = 0x94;
        pchMessageStart[1] = 0x41;
        pchMessageStart[2] = 0x2a;
        pchMessageStart[3] = 0x3f;
        vAlertPubKey = ParseHex("0470278d0645942e9816abfb0596ddb92c9e15f4efcb59d05f46579398de5f0cbc73c5dad1bf3078d26b7eff021c5628140933a8cfc430ab7c00276304d7353d9e");
        vUCCDevKey = ParseHex("0329b41789e8fd75dc7168d05dec322c25df364f6b010fb59c96b4637e5f4487cb"); // TEAMPubKey for fees
        vUCCFundKey = ParseHex("031f3b25791150d4243608c51f39c13a5b340cb73e4bf44c4d0258ad65506cd6c2"); // SWAPPubKey for fees
        nDevFee = 3; // TEAMFee %
        nFundFee = 7; // SWAPFee %
        nDefaultPort = 41112;
        bnProofOfWorkLimit = ~uint256(0) >> 20;
        bnStartWork = ~uint256(0) >> 24;

        subsidySwitchPoints = {
            {0         ,   4 * COIN},
            {20   * 1e9,   5 * COIN},
            {30   * 1e9,   7 * COIN},
            {50   * 1e9,  10 * COIN},
            {80   * 1e9,  14 * COIN},
            {130  * 1e9,  19 * COIN},
            {210  * 1e9,  25 * COIN},
            {340  * 1e9,  32 * COIN},
            {550  * 1e9,  40 * COIN},
            {890  * 1e9,  49 * COIN},
            {1440 * 1e9,  59 * COIN},
            {2330 * 1e9,  70 * COIN},
            {3770 * 1e9,  82 * COIN},
            {6100 * 1e9,  95 * COIN},
            {9870 * 1e9, 109 * COIN},
        };
        assert(subsidySwitchPoints.size());

        nMaxReorganizationDepth = 100;
        nEnforceBlockUpgradeMajority = 750;
        nRejectBlockOutdatedMajority = 950;
        nToCheckBlockUpgradeMajority = 1000;
        nMinerThreads = 0;
        nTargetSpacing = 1 * 60;  // UCC: 1 minute
        nAntiInstamineTime = 720; // 720 blocks with 1 reward for instamine prevention
        nMaturity = 60;
        nMasternodePercentDrift = 3;
        nMaxMoneyOut = 45000000 * COIN;

        nStartMasternodePaymentsBlock = 7001;

        /** Height or Time Based Activations **/
        nLastPOWBlock = 777000;
        nModifierUpdateBlock = std::numeric_limits<decltype(nModifierUpdateBlock)>::max();

        const char* pszTimestamp = "One World United! 2018-09-09";
        CMutableTransaction txNew;
        txNew.vin.resize(1);
        txNew.vout.resize(1);
        txNew.vin[0].scriptSig = CScript() << 486604799 << CScriptNum(4) << vector<unsigned char>((const unsigned char*)pszTimestamp, (const unsigned char*)pszTimestamp + strlen(pszTimestamp));
        txNew.vout[0].nValue = 50 * COIN;
        txNew.vout[0].scriptPubKey = CScript() << ParseHex("047601fdaf49621e1c2ae88272c8cf8367d783d44584ef5b029161a741ece7476f15457a410080d0b454e3033f6db0487bd368d663d0333a57312da17855287863") << OP_CHECKSIG;
        genesis.vtx.push_back(txNew);
        genesis.hashPrevBlock = 0;
        genesis.hashMerkleRoot = genesis.BuildMerkleTree();
        genesis.nVersion = 1;
        genesis.nTime = 1536512400;
        genesis.nBits = 0x1e0ffff0;
        genesis.nNonce = 1226015;

        //MineGenesis(genesis);
        //std::cout << genesis.ToString() << std::endl;

	hashGenesisBlock = genesis.GetHash();

        assert(hashGenesisBlock == uint256("000000c914b2c4a05d8e28f4ec8498b7f13798cd3513cf856c0da23690c00a89"));
        assert(genesis.hashMerkleRoot == uint256("08b1bee241c6a39de8adb7de82bedeaa139976613b67d98d12c25de5bf8681c4"));

        vSeeds.push_back(CDNSSeedData("uccnetwork.org", "seednode1.uccnetwork.org"));     // Primary DNS Seeder
	vSeeds.push_back(CDNSSeedData("uccnetwork.org", "seednode2.uccnetwork.org"));
        vSeeds.push_back(CDNSSeedData("uccnetwork.org", "seednode3.uccnetwork.org"));
        vSeeds.push_back(CDNSSeedData("50.59.59.250", "50.59.59.250"));
        vSeeds.push_back(CDNSSeedData("108.61.148.90", "108.61.148.90"));
        vSeeds.push_back(CDNSSeedData("62.113.206.204", "62.113.206.204"));

        base58Prefixes[PUBKEY_ADDRESS] = std::vector<unsigned char>(1, 68);
        base58Prefixes[SCRIPT_ADDRESS] = std::vector<unsigned char>(1, 18);
        base58Prefixes[SECRET_KEY] = std::vector<unsigned char>(1, 233);
        base58Prefixes[EXT_PUBLIC_KEY] = boost::assign::list_of(0x02)(0x3D)(0x35)(0x37).convert_to_container<std::vector<unsigned char> >();
        base58Prefixes[EXT_SECRET_KEY] = boost::assign::list_of(0x02)(0x31)(0x51)(0x2A).convert_to_container<std::vector<unsigned char> >();
        // BIP44 coin type is from https://github.com/satoshilabs/slips/blob/master/slip-0044.md
        base58Prefixes[EXT_COIN_TYPE] = boost::assign::list_of(0x80)(0x00)(0x07)(0x99).convert_to_container<std::vector<unsigned char> >();

        convertSeed6(vFixedSeeds, pnSeed6_main, ARRAYLEN(pnSeed6_main));

        fRequireRPCPassword = true;
        fMiningRequiresPeers = true;
        fDefaultConsistencyChecks = false;
        fRequireStandard = true;
        fMineBlocksOnDemand = false;
        fSkipProofOfWorkCheck = false;
        fTestnetToBeDeprecatedFieldRPC = false;
        fHeadersFirstSyncingActive = false;

        nPoolMaxTransactions = 3;
        strSporkKey = "0420c47e51a23417ac6950b562cd1e0e0a934258e4af5cc5d893820e1b5bad6bb7c5eec80ff8fcbfd6a60d43dede65c94b8c3ea7d2ce51d2baf5a03fcdc665fe93";
        strObfuscationPoolDummyAddress = "UPGfxo1fS8hrsowprmo8gpJAvmAd1N35cp";
        nStartMasternodePayments = 1536512409;
    }

    const Checkpoints::CCheckpointData& Checkpoints() const
    {
        return data;
    }
};
static CMainParams mainParams;

/**
 * Testnet (v3)
 */
class CTestNetParams : public CMainParams
{
public:
    CTestNetParams()
    {
        networkID = CBaseChainParams::TESTNET;
        strNetworkID = "test";
        pchMessageStart[0] = 0xa2;
        pchMessageStart[1] = 0x59;
        pchMessageStart[2] = 0xa9;
        pchMessageStart[3] = 0x9a;

        bnProofOfWorkLimit = ~uint256(0) >> 1;
        bnStartWork = bnProofOfWorkLimit;

        subsidySwitchPoints = {
           {0        ,   4 * COIN},
           {2   * 1e7,   5 * COIN},
           {3   * 1e7,   7 * COIN},
           {5   * 1e7,   9 * COIN},
           {8   * 1e7,  11 * COIN},
           {13  * 1e7,  15 * COIN},
           {21  * 1e7,  20 * COIN},
           {34  * 1e7,  27 * COIN},
           {55  * 1e7,  39 * COIN},
           {89  * 1e7,  57 * COIN},
           {144 * 1e7,  85 * COIN},
           {233 * 1e7, 131 * COIN},
           {377 * 1e7, 204 * COIN},
           {610 * 1e7, 321 * COIN},
           {987 * 1e7, 511 * COIN},
        };
        assert(subsidySwitchPoints.size());

        vAlertPubKey = ParseHex("0470278d0645942e9816abfb0596ddb92c9e15f4efcb59d05f46579398de5f0cbc73c5dad1bf3078d26b7eff021c5628140933a8cfc430ab7c00276304d7353d9e");
        nDefaultPort = 51112;
        nEnforceBlockUpgradeMajority = 51;
        nRejectBlockOutdatedMajority = 75;
        nToCheckBlockUpgradeMajority = 100;
        nMinerThreads = 0;
        nTargetSpacing = 1 * 10;  // UCC: 10 seconds
        nLastPOWBlock = 1500;
        nMaturity = 15;
        nMasternodePercentDrift = 4;
        nModifierUpdateBlock = std::numeric_limits<decltype(nModifierUpdateBlock)>::max();
        nMaxMoneyOut = 1000000000 * COIN;

	nStartMasternodePaymentsBlock = 1001;


        //! Modify the testnet genesis block so the timestamp is valid for a later start.
        genesis.nTime = 1559125649;
        genesis.nNonce = 884877;

	//MineGenesis(genesis);

        hashGenesisBlock = genesis.GetHash();

        assert(hashGenesisBlock == uint256("00000bd6961045157f04144acb30a3d8e012f0a8171b63fa11e3f06fef627a3d"));

        vFixedSeeds.clear();
        vSeeds.clear();
        vSeeds.push_back(CDNSSeedData("uccnetwork.org", "seednode1.uccnetwork.org"));     // Primary DNS Seeder
	vSeeds.push_back(CDNSSeedData("uccnetwork.org", "seednode2.uccnetwork.org"));
        vSeeds.push_back(CDNSSeedData("uccnetwork.org", "seednode3.uccnetwork.org"));

        base58Prefixes[PUBKEY_ADDRESS] = std::vector<unsigned char>(1, 128); // Testnet UCC addresses start with 't'
        base58Prefixes[SCRIPT_ADDRESS] = std::vector<unsigned char>(1, 11);
        base58Prefixes[SECRET_KEY] = std::vector<unsigned char>(1, 240);     // Testnet private keys start with '9' or 'c' (Bitcoin defaults)
        // Testnet UCC BIP32 pubkeys start with 'DRKV'
        base58Prefixes[EXT_PUBLIC_KEY] = boost::assign::list_of(0x3a)(0x80)(0x51)(0xc0).convert_to_container<std::vector<unsigned char> >();
        // Testnet UCC BIP32 prvkeys start with 'DRKP'
        base58Prefixes[EXT_SECRET_KEY] = boost::assign::list_of(0x3a)(0x81)(0x88)(0xf7).convert_to_container<std::vector<unsigned char> >();
        // Testnet UCC BIP44 coin type is '1' (All coin's testnet default)
        base58Prefixes[EXT_COIN_TYPE] = boost::assign::list_of(0x80)(0x00)(0x00)(0x01).convert_to_container<std::vector<unsigned char> >();

        convertSeed6(vFixedSeeds, pnSeed6_test, ARRAYLEN(pnSeed6_test));

        fRequireRPCPassword = true;
        fMiningRequiresPeers = false;
        fDefaultConsistencyChecks = false;
        fRequireStandard = false;
        fMineBlocksOnDemand = false;
        fTestnetToBeDeprecatedFieldRPC = true;

        nPoolMaxTransactions = 2;
        strSporkKey = "04cce5d10552bda19b62b597f80f37d8ad4c2c28b59d5bc0e39e42c046fbb73522a6b23339ffcd43f3aba62c128486c2e74b004480ed977c09252c78150bcb2d7a";
        strObfuscationPoolDummyAddress = "tk7oN9aE8Foa8gqoJCYx4UcrWnEaFvr3Co";
        nStartMasternodePayments = 1558695858;
    }
    const Checkpoints::CCheckpointData& Checkpoints() const
    {
        return dataTestnet;
    }
};
static CTestNetParams testNetParams;

/**
 * Regression test
 */
class CRegTestParams : public CTestNetParams
{
public:
    CRegTestParams()
    {
        networkID = CBaseChainParams::REGTEST;
        strNetworkID = "regtest";
        pchMessageStart[0] = 0xa4;
        pchMessageStart[1] = 0x5f;
        pchMessageStart[2] = 0x7e;
        pchMessageStart[3] = 0x2c;

        bnStartWork = ~uint256(0) >> 20;

        subsidySwitchPoints = {
           {0        ,   4 * COIN},
           {2   * 1e7,   5 * COIN},
           {3   * 1e7,   7 * COIN},
           {5   * 1e7,   9 * COIN},
           {8   * 1e7,  11 * COIN},
           {13  * 1e7,  15 * COIN},
           {21  * 1e7,  20 * COIN},
           {34  * 1e7,  27 * COIN},
           {55  * 1e7,  39 * COIN},
           {89  * 1e7,  57 * COIN},
           {144 * 1e7,  85 * COIN},
           {233 * 1e7, 131 * COIN},
           {377 * 1e7, 204 * COIN},
           {610 * 1e7, 321 * COIN},
           {987 * 1e7, 511 * COIN},
        };
        assert(subsidySwitchPoints.size());

        nEnforceBlockUpgradeMajority = 750;
        nRejectBlockOutdatedMajority = 950;
        nToCheckBlockUpgradeMajority = 1000;
        nMinerThreads = 1;
        nTargetSpacing = 1 * 60;        // UCC: 1 minute
        bnProofOfWorkLimit = ~uint256(0) >> 1;
        genesis.nTime = 1536512402;
        genesis.nBits = 0x207fffff;
        genesis.nNonce = 3909880;

        hashGenesisBlock = genesis.GetHash();
        nDefaultPort = 61112;

	//MineGenesis(genesis);
        //std::cout << genesis.ToString() << std::endl;

        assert(hashGenesisBlock == uint256("0000065c21928f425a473b9e61b4f25865077176507d5b80ed4260bb4f8b1a89"));

        vFixedSeeds.clear(); //! Testnet mode doesn't have any fixed seeds.
        vSeeds.clear();      //! Testnet mode doesn't have any DNS seeds.

        fRequireRPCPassword = false;
        fMiningRequiresPeers = false;
        fDefaultConsistencyChecks = true;
        fRequireStandard = false;
        fMineBlocksOnDemand = true;
        fTestnetToBeDeprecatedFieldRPC = false;
    }
    const Checkpoints::CCheckpointData& Checkpoints() const
    {
        return dataRegtest;
    }
};
static CRegTestParams regTestParams;

/**
 * Unit test
 */
class CUnitTestParams : public CMainParams, public CModifiableParams
{
public:
    CUnitTestParams()
    {
        networkID = CBaseChainParams::UNITTEST;
        strNetworkID = "unittest";
        nDefaultPort = 31112;
        vFixedSeeds.clear(); //! Unit test mode doesn't have any fixed seeds.
        vSeeds.clear();      //! Unit test mode doesn't have any DNS seeds.

        fRequireRPCPassword = false;
        fMiningRequiresPeers = false;
        fDefaultConsistencyChecks = true;
        fMineBlocksOnDemand = true;

        subsidySwitchPoints = {
            {0         ,   1 * COIN},
            {2   * 1e5,   2 * COIN},
            {3   * 1e5,   3 * COIN},
            {5   * 1e5,   5 * COIN},
            {8   * 1e5,   8 * COIN},
            {13  * 1e5,  13 * COIN},
            {21  * 1e5,  21 * COIN},
            {34  * 1e5,  34 * COIN},
            {55  * 1e5,  55 * COIN},
            {89  * 1e5,  89 * COIN},
            {144 * 1e5, 144 * COIN},
            {233 * 1e5, 233 * COIN},
            {377 * 1e5, 377 * COIN},
            {610 * 1e5, 610 * COIN},
            {987 * 1e5, 987 * COIN},
        };
        assert(subsidySwitchPoints.size());

    }

    const Checkpoints::CCheckpointData& Checkpoints() const
    {
        // UnitTest share the same checkpoints as MAIN
        return data;
    }

    //! Published setters to allow changing values in unit test cases
    virtual void setEnforceBlockUpgradeMajority(int anEnforceBlockUpgradeMajority) { nEnforceBlockUpgradeMajority = anEnforceBlockUpgradeMajority; }
    virtual void setRejectBlockOutdatedMajority(int anRejectBlockOutdatedMajority) { nRejectBlockOutdatedMajority = anRejectBlockOutdatedMajority; }
    virtual void setToCheckBlockUpgradeMajority(int anToCheckBlockUpgradeMajority) { nToCheckBlockUpgradeMajority = anToCheckBlockUpgradeMajority; }
    virtual void setDefaultConsistencyChecks(bool afDefaultConsistencyChecks) { fDefaultConsistencyChecks = afDefaultConsistencyChecks; }
    virtual void setSkipProofOfWorkCheck(bool afSkipProofOfWorkCheck) { fSkipProofOfWorkCheck = afSkipProofOfWorkCheck; }
};
static CUnitTestParams unitTestParams;


static CChainParams* pCurrentParams = 0;

CModifiableParams* ModifiableParams()
{
    assert(pCurrentParams);
    assert(pCurrentParams == &unitTestParams);
    return (CModifiableParams*)&unitTestParams;
}

const CChainParams& Params()
{
    assert(pCurrentParams);
    return *pCurrentParams;
}

CChainParams& Params(CBaseChainParams::Network network)
{
    switch (network) {
    case CBaseChainParams::MAIN:
        return mainParams;
    case CBaseChainParams::TESTNET:
        return testNetParams;
    case CBaseChainParams::REGTEST:
        return regTestParams;
    case CBaseChainParams::UNITTEST:
        return unitTestParams;
    default:
        assert(false && "Unimplemented network");
        return mainParams;
    }
}

void SelectParams(CBaseChainParams::Network network)
{
    SelectBaseParams(network);
    pCurrentParams = &Params(network);
}

bool SelectParamsFromCommandLine()
{
    CBaseChainParams::Network network = NetworkIdFromCommandLine();
    if (network == CBaseChainParams::MAX_NETWORK_TYPES)
        return false;

    SelectParams(network);
    return true;
}

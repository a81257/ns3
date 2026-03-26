// SPDX-License-Identifier: GPL-2.0-only
/**
 * @file ub-test.cc
 * @brief Test suite for the unified-bus module
 * 
 * This file contains unit tests for the unified-bus functionality,
 * including basic object creation, configuration, and core features.
 */

#include "ns3/test.h"
#include "ns3/ub-app.h"
#include "ns3/ub-traffic-gen.h"
#include "ns3/log.h"
#include "ns3/simulator.h"
#include "ns3/config.h"
#include "ns3/rng-seed-manager.h"
#include "ns3/node-container.h"
#include "ns3/ub-utils.h"
#include "ns3/ub-controller.h"

#include <chrono>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <sstream>

using namespace ns3;

NS_LOG_COMPONENT_DEFINE("UbTest");

/**
 * @brief Unified-bus functionality test
 * 
 * Tests basic unified-bus module functionality including:
 * - Object creation and initialization
 * - Singleton pattern verification
 * - Configuration system integration
 * - Basic API functionality
 */
class UbFunctionalityTest : public TestCase
{
public:
    UbFunctionalityTest();
    void DoRun() override;
    
private:
    void DoSetup() override;
    void DoTeardown() override;
};

UbFunctionalityTest::UbFunctionalityTest()
    : TestCase("UnifiedBus - Core functionality test")
{
}

void UbFunctionalityTest::DoSetup()
{
    Config::Reset();
    RngSeedManager::SetSeed(12345);
}

void UbFunctionalityTest::DoTeardown()
{
    // Minimal cleanup
    if (!Simulator::IsFinished()) {
        Simulator::Destroy();
    }
}

void UbFunctionalityTest::DoRun()
{
    NS_LOG_FUNCTION(this);
    
    // Test 1: UbTrafficGen singleton
    UbTrafficGen& gen1 = UbTrafficGen::GetInstance();
    UbTrafficGen& gen2 = UbTrafficGen::GetInstance();
    NS_TEST_ASSERT_MSG_EQ(&gen1, &gen2, "UbTrafficGen should be singleton");
    
    // Test 2: Initial state
    NS_TEST_ASSERT_MSG_EQ(gen1.IsCompleted(), true, "UbTrafficGen should be completed initially");
    
    // Test 3: UbApp creation
    Ptr<UbApp> app = CreateObject<UbApp>();
    NS_TEST_ASSERT_MSG_NE(app, nullptr, "UbApp creation should succeed");
    
    // Test 4: Node creation
    NodeContainer nodes;
    nodes.Create(2);
    NS_TEST_ASSERT_MSG_EQ(nodes.GetN(), 2, "Should create 2 nodes");
    
    // Test 5: Configuration setting (without getting)
    Config::SetDefault("ns3::UbApp::EnableMultiPath", BooleanValue(false));
    Config::SetDefault("ns3::UbPort::UbDataRate", StringValue("400Gbps"));
    
    NS_LOG_INFO("All basic tests completed successfully");
}

class UbCreateNodeSystemIdTest : public TestCase
{
  public:
    UbCreateNodeSystemIdTest()
        : TestCase("UnifiedBus - CreateNode honors systemId column")
    {
    }

    void DoRun() override
    {
        namespace fs = std::filesystem;

        const uint32_t beforeNodes = NodeList::GetNNodes();
        auto uniqueSuffix = std::to_string(
            std::chrono::steady_clock::now().time_since_epoch().count());
        fs::path caseDir = fs::temp_directory_path() / ("ub-systemid-test-" + uniqueSuffix);
        std::error_code ec;
        fs::remove_all(caseDir, ec);
        ec.clear();
        fs::create_directories(caseDir, ec);
        NS_TEST_ASSERT_MSG_EQ(ec.value(), 0, "Temporary case directory creation should succeed");

        fs::path nodePath = caseDir / "node.csv";
        std::ofstream nodeFile(nodePath.string());
        nodeFile << "nodeId,nodeType,portNum,forwardDelay,systemId\n";
        nodeFile << "0,DEVICE,1,1ns,0\n";
        nodeFile << "1,SWITCH,2,1ns,1\n";
        nodeFile.close();

        utils::UbUtils::Get()->CreateNode(nodePath.string());

        NS_TEST_ASSERT_MSG_EQ(NodeList::GetNNodes(), beforeNodes + 2, "CreateNode should create 2 nodes");
        NS_TEST_ASSERT_MSG_EQ(NodeList::GetNode(beforeNodes)->GetSystemId(), 0u,
                              "First created node should preserve systemId 0");
        NS_TEST_ASSERT_MSG_EQ(NodeList::GetNode(beforeNodes + 1)->GetSystemId(), 1u,
                              "Second created node should preserve systemId 1");

        fs::remove_all(caseDir, ec);
    }
};

#ifdef NS3_MPI
class UbCreateTopoRemoteLinkTest : public TestCase
{
  public:
    UbCreateTopoRemoteLinkTest()
        : TestCase("UnifiedBus - CreateTopo builds remote link across systemId")
    {
    }

    void DoRun() override
    {
        namespace fs = std::filesystem;

        const uint32_t beforeNodes = NodeList::GetNNodes();
        auto uniqueSuffix = std::to_string(
            std::chrono::steady_clock::now().time_since_epoch().count());
        fs::path caseDir = fs::temp_directory_path() / ("ub-remote-link-test-" + uniqueSuffix);
        std::error_code ec;
        fs::remove_all(caseDir, ec);
        ec.clear();
        fs::create_directories(caseDir, ec);
        NS_TEST_ASSERT_MSG_EQ(ec.value(), 0, "Temporary case directory creation should succeed");

        fs::path nodePath = caseDir / "node.csv";
        const uint32_t node0Id = beforeNodes;
        const uint32_t node1Id = beforeNodes + 1;

        std::ofstream nodeFile(nodePath.string());
        nodeFile << "nodeId,nodeType,portNum,forwardDelay,systemId\n";
        nodeFile << node0Id << ",DEVICE,1,1ns,0\n";
        nodeFile << node1Id << ",DEVICE,1,1ns,1\n";
        nodeFile.close();

        fs::path topoPath = caseDir / "topology.csv";
        std::ofstream topoFile(topoPath.string());
        topoFile << "node1,port1,node2,port2,bandwidth,delay\n";
        topoFile << node0Id << ",0," << node1Id << ",0,400Gbps,10ns\n";
        topoFile.close();

        utils::UbUtils::Get()->CreateNode(nodePath.string());
        utils::UbUtils::Get()->CreateTopo(topoPath.string());

        NS_TEST_ASSERT_MSG_EQ(NodeList::GetNNodes(), beforeNodes + 2, "CreateNode should create 2 nodes");

        Ptr<Node> n0 = NodeList::GetNode(beforeNodes);
        Ptr<Node> n1 = NodeList::GetNode(beforeNodes + 1);
        Ptr<UbPort> p0 = DynamicCast<UbPort>(n0->GetDevice(0));
        Ptr<UbPort> p1 = DynamicCast<UbPort>(n1->GetDevice(0));
        Ptr<Channel> channel = p0->GetChannel();

        NS_TEST_ASSERT_MSG_NE(channel, nullptr, "Port channel should be created");
        NS_TEST_ASSERT_MSG_EQ(channel->GetInstanceTypeId().GetName(), std::string("ns3::UbRemoteLink"),
                              "Cross-systemId topology should use UbRemoteLink");
        NS_TEST_ASSERT_MSG_EQ(p0->HasMpiReceive(), true, "Remote link endpoint should enable MPI receive");
        NS_TEST_ASSERT_MSG_EQ(p1->HasMpiReceive(), true, "Remote link endpoint should enable MPI receive");
        NS_TEST_ASSERT_MSG_EQ(p1->GetChannel(), p0->GetChannel(), "Both ports should share the same link");

        fs::remove_all(caseDir, ec);
    }
};
#endif

#if defined(NS3_MPI) && defined(NS3_MTP)
class UbCreateTopoPackedSystemIdLocalLinkTest : public TestCase
{
  public:
    UbCreateTopoPackedSystemIdLocalLinkTest()
        : TestCase("UnifiedBus - CreateTopo keeps same-rank packed systemId on local link")
    {
    }

    void DoRun() override
    {
        namespace fs = std::filesystem;

        const uint32_t beforeNodes = NodeList::GetNNodes();
        auto uniqueSuffix = std::to_string(
            std::chrono::steady_clock::now().time_since_epoch().count());
        fs::path caseDir = fs::temp_directory_path() / ("ub-packed-local-link-test-" + uniqueSuffix);
        std::error_code ec;
        fs::remove_all(caseDir, ec);
        ec.clear();
        fs::create_directories(caseDir, ec);
        NS_TEST_ASSERT_MSG_EQ(ec.value(), 0, "Temporary case directory creation should succeed");

        const uint32_t node0Id = beforeNodes;
        const uint32_t node1Id = beforeNodes + 1;
        const uint32_t node0SystemId = (0x0001u << 16) | 0x0009u;
        const uint32_t node1SystemId = (0x0002u << 16) | 0x0009u;

        fs::path nodePath = caseDir / "node.csv";
        std::ofstream nodeFile(nodePath.string());
        nodeFile << "nodeId,nodeType,portNum,forwardDelay,systemId\n";
        nodeFile << node0Id << ",DEVICE,1,1ns," << node0SystemId << "\n";
        nodeFile << node1Id << ",DEVICE,1,1ns," << node1SystemId << "\n";
        nodeFile.close();

        fs::path topoPath = caseDir / "topology.csv";
        std::ofstream topoFile(topoPath.string());
        topoFile << "node1,port1,node2,port2,bandwidth,delay\n";
        topoFile << node0Id << ",0," << node1Id << ",0,400Gbps,10ns\n";
        topoFile.close();

        utils::UbUtils::Get()->CreateNode(nodePath.string());
        utils::UbUtils::Get()->CreateTopo(topoPath.string());

        Ptr<Node> n0 = NodeList::GetNode(beforeNodes);
        Ptr<Node> n1 = NodeList::GetNode(beforeNodes + 1);
        Ptr<UbPort> p0 = DynamicCast<UbPort>(n0->GetDevice(0));
        Ptr<UbPort> p1 = DynamicCast<UbPort>(n1->GetDevice(0));
        Ptr<Channel> channel = p0->GetChannel();

        NS_TEST_ASSERT_MSG_NE(channel, nullptr, "Port channel should be created");
        NS_TEST_ASSERT_MSG_EQ(channel->GetInstanceTypeId().GetName(), std::string("ns3::UbLink"),
                              "Same MPI rank packed systemId should keep a local UbLink");
        NS_TEST_ASSERT_MSG_EQ(p0->HasMpiReceive(), false,
                              "Local link should not enable MPI receive on the first endpoint");
        NS_TEST_ASSERT_MSG_EQ(p1->HasMpiReceive(), false,
                              "Local link should not enable MPI receive on the second endpoint");
        NS_TEST_ASSERT_MSG_EQ(p1->GetChannel(), p0->GetChannel(), "Both ports should share the same local link");

        fs::remove_all(caseDir, ec);
    }
};
#endif

class UbCreateTpPreloadInstancesTest : public TestCase
{
  public:
    UbCreateTpPreloadInstancesTest()
        : TestCase("UnifiedBus - CreateTp preloads TP instances from config")
    {
    }

    void DoRun() override
    {
        namespace fs = std::filesystem;

        const uint32_t beforeNodes = NodeList::GetNNodes();
        auto uniqueSuffix = std::to_string(
            std::chrono::steady_clock::now().time_since_epoch().count());
        fs::path caseDir = fs::temp_directory_path() / ("ub-create-tp-test-" + uniqueSuffix);
        std::error_code ec;
        fs::remove_all(caseDir, ec);
        ec.clear();
        fs::create_directories(caseDir, ec);
        NS_TEST_ASSERT_MSG_EQ(ec.value(), 0, "Temporary case directory creation should succeed");

        const uint32_t node0Id = beforeNodes;
        const uint32_t node1Id = beforeNodes + 1;

        fs::path nodePath = caseDir / "node.csv";
        std::ofstream nodeFile(nodePath.string());
        nodeFile << "nodeId,nodeType,portNum,forwardDelay,systemId\n";
        nodeFile << node0Id << ",DEVICE,1,1ns,0\n";
        nodeFile << node1Id << ",DEVICE,1,1ns,1\n";
        nodeFile.close();

        fs::path tpPath = caseDir / "transport_channel.csv";
        std::ofstream tpFile(tpPath.string());
        tpFile << "nodeId1,portId1,tpn1,nodeId2,portId2,tpn2,priority,metric\n";
        tpFile << node0Id << ",0,11," << node1Id << ",0,22,7,1\n";
        tpFile.close();

        utils::UbUtils::Get()->CreateNode(nodePath.string());
        utils::UbUtils::Get()->CreateTp(tpPath.string());

        Ptr<Node> n0 = NodeList::GetNode(beforeNodes);
        Ptr<Node> n1 = NodeList::GetNode(beforeNodes + 1);
        Ptr<UbController> c0 = n0->GetObject<UbController>();
        Ptr<UbController> c1 = n1->GetObject<UbController>();

        NS_TEST_ASSERT_MSG_EQ(c0->IsTPExists(11), true, "Source-side TP should be preloaded from config");
        NS_TEST_ASSERT_MSG_EQ(c1->IsTPExists(22), true, "Destination-side TP should be preloaded from config");

        fs::remove_all(caseDir, ec);
    }
};

class UbTraceDirSetupTest : public TestCase
{
  public:
    UbTraceDirSetupTest()
        : TestCase("UnifiedBus - Trace directory setup tolerates missing runlog")
    {
    }

    void DoRun() override
    {
        namespace fs = std::filesystem;

        auto uniqueSuffix = std::to_string(
            std::chrono::steady_clock::now().time_since_epoch().count());
        fs::path caseDir = fs::temp_directory_path() / ("ub-trace-dir-test-" + uniqueSuffix);
        std::error_code ec;
        fs::remove_all(caseDir, ec);
        ec.clear();
        fs::create_directories(caseDir, ec);
        NS_TEST_ASSERT_MSG_EQ(ec.value(), 0, "Temporary case directory creation should succeed");

        fs::path configPath = caseDir / "network_attribute.txt";
        std::string tracePath = utils::UbUtils::PrepareTraceDir(configPath.string());

        NS_TEST_ASSERT_MSG_EQ(fs::exists(caseDir / "runlog"), true, "runlog directory should be created");
        NS_TEST_ASSERT_MSG_EQ(tracePath.empty(), false, "Returned trace path should not be empty");

        std::ofstream staleFile((caseDir / "runlog" / "stale.tr").string());
        staleFile << "stale";
        staleFile.close();

        tracePath = utils::UbUtils::PrepareTraceDir(configPath.string());
        NS_TEST_ASSERT_MSG_EQ(fs::exists(caseDir / "runlog" / "stale.tr"), false, "Existing runlog contents should be removed");
        NS_TEST_ASSERT_MSG_EQ(fs::exists(caseDir / "runlog"), true, "runlog directory should be recreated");

        fs::remove_all(caseDir, ec);
    }
};

class UbMpiRankExtractionHelperTest : public TestCase
{
  public:
    UbMpiRankExtractionHelperTest()
        : TestCase("UnifiedBus - ExtractMpiRank follows MPI rank encoding rules")
    {
    }

    void DoRun() override
    {
        NS_TEST_ASSERT_MSG_EQ(utils::UbUtils::ExtractMpiRank(7u),
                              7u,
                              "Plain systemId should preserve rank value");

        const uint32_t packedSystemId = (0x1234u << 16) | 0x002au;
#ifdef NS3_MTP
        NS_TEST_ASSERT_MSG_EQ(utils::UbUtils::ExtractMpiRank(packedSystemId),
                              0x002au,
                              "MTP packed systemId should use low 16 bits as MPI rank");
#else
        NS_TEST_ASSERT_MSG_EQ(utils::UbUtils::ExtractMpiRank(packedSystemId),
                              packedSystemId,
                              "Non-MTP build should use the full systemId as MPI rank");
#endif
    }
};

class UbSameMpiRankHelperTest : public TestCase
{
  public:
    UbSameMpiRankHelperTest()
        : TestCase("UnifiedBus - IsSameMpiRank compares MPI rank instead of raw packed systemId")
    {
    }

    void DoRun() override
    {
        NS_TEST_ASSERT_MSG_EQ(utils::UbUtils::IsSameMpiRank(5u, 5u),
                              true,
                              "Identical plain systemId values should be on the same MPI rank");
        NS_TEST_ASSERT_MSG_EQ(utils::UbUtils::IsSameMpiRank(5u, 6u),
                              false,
                              "Different plain systemId values should be on different MPI ranks");

        const uint32_t lhsPacked = (0x0001u << 16) | 0x0009u;
        const uint32_t rhsSameRankPacked = (0x0002u << 16) | 0x0009u;
        const uint32_t rhsDifferentRankPacked = (0x0002u << 16) | 0x000au;

#ifdef NS3_MTP
        NS_TEST_ASSERT_MSG_EQ(utils::UbUtils::IsSameMpiRank(lhsPacked, rhsSameRankPacked),
                              true,
                              "MTP packed systemId values with the same low 16 bits should match");
#else
        NS_TEST_ASSERT_MSG_EQ(utils::UbUtils::IsSameMpiRank(lhsPacked, rhsSameRankPacked),
                              false,
                              "Non-MTP build should compare full systemId values");
#endif
        NS_TEST_ASSERT_MSG_EQ(utils::UbUtils::IsSameMpiRank(lhsPacked, rhsDifferentRankPacked),
                              false,
                              "Different MPI rank encodings should not match");
    }
};

class UbSystemOwnedByRankHelperTest : public TestCase
{
  public:
    UbSystemOwnedByRankHelperTest()
        : TestCase("UnifiedBus - IsSystemOwnedByRank follows packed MPI ownership rules")
    {
    }

    void DoRun() override
    {
        NS_TEST_ASSERT_MSG_EQ(utils::UbUtils::IsSystemOwnedByRank(7u, 7u),
                              true,
                              "Plain systemId should be owned by the same MPI rank");
        NS_TEST_ASSERT_MSG_EQ(utils::UbUtils::IsSystemOwnedByRank(7u, 6u),
                              false,
                              "Plain systemId should not be owned by a different MPI rank");

        const uint32_t packedSystemId = (0x1234u << 16) | 0x0009u;

#ifdef NS3_MTP
        NS_TEST_ASSERT_MSG_EQ(utils::UbUtils::IsSystemOwnedByRank(packedSystemId, 0x0009u),
                              true,
                              "Packed systemId should be owned by the matching low-16-bit MPI rank");
        NS_TEST_ASSERT_MSG_EQ(utils::UbUtils::IsSystemOwnedByRank(packedSystemId, 0x000au),
                              false,
                              "Packed systemId should not be owned by a different low-16-bit MPI rank");
#else
        NS_TEST_ASSERT_MSG_EQ(utils::UbUtils::IsSystemOwnedByRank(packedSystemId, packedSystemId),
                              true,
                              "Non-MTP build should treat the full systemId as the owner key");
        NS_TEST_ASSERT_MSG_EQ(utils::UbUtils::IsSystemOwnedByRank(packedSystemId, 0x0009u),
                              false,
                              "Non-MTP build should not mask packed systemId values");
#endif
    }
};

/**
 * @brief Unified-bus test suite
 */
class UbTestSuite : public TestSuite
{
public:
    UbTestSuite();
};

UbTestSuite::UbTestSuite()
    : TestSuite("unified-bus", Type::UNIT)
{
    AddTestCase(new UbFunctionalityTest(), TestCase::Duration::QUICK);
    AddTestCase(new UbTraceDirSetupTest(), TestCase::Duration::QUICK);
    AddTestCase(new UbMpiRankExtractionHelperTest(), TestCase::Duration::QUICK);
    AddTestCase(new UbSameMpiRankHelperTest(), TestCase::Duration::QUICK);
    AddTestCase(new UbSystemOwnedByRankHelperTest(), TestCase::Duration::QUICK);
    AddTestCase(new UbCreateNodeSystemIdTest(), TestCase::Duration::QUICK);
#ifdef NS3_MPI
    AddTestCase(new UbCreateTopoRemoteLinkTest(), TestCase::Duration::QUICK);
#endif
#if defined(NS3_MPI) && defined(NS3_MTP)
    AddTestCase(new UbCreateTopoPackedSystemIdLocalLinkTest(), TestCase::Duration::QUICK);
#endif
    AddTestCase(new UbCreateTpPreloadInstancesTest(), TestCase::Duration::QUICK);
}

// Register the test suite
static UbTestSuite g_ubTestSuite;

namespace
{

std::filesystem::path
LocateRepoRoot();

std::pair<int, std::string>
RunQuickExampleCommand(const std::string& testFile,
                       const std::string& extraArgs,
                       const std::string& commandPrefix,
                       const std::string& casePathRelative = "");

std::pair<int, std::string>
RunNs3RunCommand(const std::string& testFile,
                 const std::string& programAndArgs,
                 const std::string& commandPrefix = "");

} // namespace

#ifdef NS3_MTP
class UbQuickExampleLocalMtpSystemTest : public TestCase
{
  public:
    UbQuickExampleLocalMtpSystemTest()
        : TestCase("UnifiedBus - ub-quick-example local MTP mode runs without MPI init failure")
    {
    }

    void DoRun() override
    {
        SetDataDir(NS_TEST_SOURCEDIR);
        auto [status, output] =
            RunQuickExampleCommand(CreateTempDirFilename("ub-quick-example-local-mtp.log"),
                                   "--mtp-threads=2",
                                   "",
                                   "scratch/2nodes_single-tp");

        NS_TEST_ASSERT_MSG_EQ(status,
                              0,
                              "ub-quick-example local MTP mode should exit successfully");
        NS_TEST_ASSERT_MSG_EQ(output.find("MPI_Testany() ... before MPI_INIT"), std::string::npos,
                              "ub-quick-example local MTP mode should not touch MPI before MPI_Init");
    }
};
#endif

#ifdef NS3_MPI
class UbQuickExampleSpoofedMpiEnvSystemTest : public TestCase
{
  public:
    UbQuickExampleSpoofedMpiEnvSystemTest()
        : TestCase("UnifiedBus - ub-quick-example ignores spoofed MPI env without launcher")
    {
    }

    void DoRun() override
    {
        SetDataDir(NS_TEST_SOURCEDIR);
        auto [status, output] =
            RunQuickExampleCommand(CreateTempDirFilename("ub-quick-example-spoofed-mpi-env.log"),
                                   "--test",
                                   "env OMPI_COMM_WORLD_SIZE=2",
                                   "scratch/2nodes_single-tp");

        NS_TEST_ASSERT_MSG_EQ(status,
                              0,
                              "spoofed MPI environment without launcher should stay on local runtime");
        NS_TEST_ASSERT_MSG_EQ(output.find(UbTrafficGen::GetMultiProcessUnsupportedMessage()),
                              std::string::npos,
                              "spoofed MPI environment should not trigger the multi-process rejection");
        NS_TEST_ASSERT_MSG_NE(output.find("TEST : 00000 : PASSED"),
                              std::string::npos,
                              "spoofed MPI environment case should still complete locally");
    }
};
#endif

namespace
{

std::filesystem::path
LocateRepoRoot()
{
    std::filesystem::path repoRoot = NS_TEST_SOURCEDIR;
    const std::filesystem::path binaryRelativePath =
        "build/src/unified-bus/examples/ns3.44-ub-quick-example-default";
    for (uint32_t i = 0; i < 4 && !std::filesystem::exists(repoRoot / binaryRelativePath); ++i)
    {
        repoRoot = repoRoot.parent_path();
    }
    return repoRoot;
}

std::pair<int, std::string>
RunQuickExampleCommand(const std::string& testFile,
                       const std::string& extraArgs,
                       const std::string& commandPrefix,
                       const std::string& casePathRelative)
{
    const std::filesystem::path repoRoot = LocateRepoRoot();
    const std::filesystem::path binaryPath =
        repoRoot / "build/src/unified-bus/examples/ns3.44-ub-quick-example-default";

    std::string command;
    if (!commandPrefix.empty())
    {
        command += commandPrefix + " ";
    }
    command += "\"" + binaryPath.string() + "\"";
    if (!casePathRelative.empty())
    {
        const std::filesystem::path casePath = repoRoot / casePathRelative;
        command += " --case-path=\"" + casePath.string() + "\"";
    }
    if (!extraArgs.empty())
    {
        command += " " + extraArgs;
    }
    command += " > \"" + testFile + "\" 2>&1";

    const int status = std::system(command.c_str());

    std::ifstream input(testFile);
    std::stringstream buffer;
    buffer << input.rdbuf();
    return {status, buffer.str()};
}

std::pair<int, std::string>
RunNs3RunCommand(const std::string& testFile,
                 const std::string& programAndArgs,
                 const std::string& commandPrefix)
{
    const std::filesystem::path repoRoot = LocateRepoRoot();

    std::string command;
    if (!commandPrefix.empty())
    {
        command += commandPrefix + " ";
    }
    command += "python3 \"" + (repoRoot / "ns3").string() + "\" run \"" + programAndArgs +
               "\" --no-build";
    command += " > \"" + testFile + "\" 2>&1";

    const int status = std::system(command.c_str());

    std::ifstream input(testFile);
    std::stringstream buffer;
    buffer << input.rdbuf();
    return {status, buffer.str()};
}

std::string
NormalizeTestPath(const std::filesystem::path& path)
{
    return std::filesystem::absolute(path).lexically_normal().string();
}

std::filesystem::path
CopyCaseDirWithoutFile(const std::string& sourceCasePathRelative, const std::string& omittedFilename)
{
    namespace fs = std::filesystem;

    const fs::path repoRoot = LocateRepoRoot();
    const fs::path sourceCaseDir = repoRoot / sourceCasePathRelative;
    const auto uniqueSuffix =
        std::to_string(std::chrono::steady_clock::now().time_since_epoch().count());
    const fs::path tempCaseDir =
        (fs::temp_directory_path() / ("ub-quick-example-case-copy-" + uniqueSuffix)).lexically_normal();

    fs::create_directories(tempCaseDir);
    for (const auto& entry : fs::directory_iterator(sourceCaseDir))
    {
        const fs::path destination = tempCaseDir / entry.path().filename();
        if (entry.path().filename() == omittedFilename)
        {
            continue;
        }
        fs::copy(entry.path(), destination, fs::copy_options::recursive);
    }

    return tempCaseDir;
}

std::filesystem::path
CopyCaseDirWithTrafficFile(const std::string& sourceCasePathRelative, const std::string& trafficCsvContent)
{
    namespace fs = std::filesystem;

    const fs::path repoRoot = LocateRepoRoot();
    const fs::path sourceCaseDir = repoRoot / sourceCasePathRelative;
    const auto uniqueSuffix =
        std::to_string(std::chrono::steady_clock::now().time_since_epoch().count());
    const fs::path tempCaseDir =
        (fs::temp_directory_path() / ("ub-quick-example-traffic-copy-" + uniqueSuffix))
            .lexically_normal();

    fs::create_directories(tempCaseDir);
    for (const auto& entry : fs::directory_iterator(sourceCaseDir))
    {
        const fs::path destination = tempCaseDir / entry.path().filename();
        fs::copy(entry.path(), destination, fs::copy_options::recursive);
    }

    std::ofstream trafficFile(tempCaseDir / "traffic.csv");
    trafficFile << trafficCsvContent;
    trafficFile.close();

    return tempCaseDir;
}

std::pair<int, std::string>
RunQuickExampleAbsoluteCaseCommand(const std::string& testFile,
                                   const std::string& extraArgs,
                                   const std::string& commandPrefix,
                                   const std::filesystem::path& casePath)
{
    const std::filesystem::path repoRoot = LocateRepoRoot();
    const std::filesystem::path binaryPath =
        repoRoot / "build/src/unified-bus/examples/ns3.44-ub-quick-example-default";

    std::string command;
    if (!commandPrefix.empty())
    {
        command += commandPrefix + " ";
    }
    command += "\"" + binaryPath.string() + "\"";
    command += " --case-path=\"" + casePath.string() + "\"";
    if (!extraArgs.empty())
    {
        command += " " + extraArgs;
    }
    command += " > \"" + testFile + "\" 2>&1";

    const int status = std::system(command.c_str());

    std::ifstream input(testFile);
    std::stringstream buffer;
    buffer << input.rdbuf();
    return {status, buffer.str()};
}

} // namespace

class UbQuickExampleMissingCasePathSystemTest : public TestCase
{
  public:
    UbQuickExampleMissingCasePathSystemTest()
        : TestCase("UnifiedBus - ub-quick-example rejects missing case-path")
    {
    }

    void DoRun() override
    {
        SetDataDir(NS_TEST_SOURCEDIR);
        auto [status, output] = RunQuickExampleCommand(CreateTempDirFilename(GetName() + ".log"),
                                                       "",
                                                       "");

        NS_TEST_ASSERT_MSG_NE(status,
                              0,
                              "ub-quick-example without case-path should exit with failure");
        NS_TEST_ASSERT_MSG_NE(output.find("missing required case path (--case-path or casePath)"),
                              std::string::npos,
                              "ub-quick-example should print a clear missing case-path error");
    }
};

class UbQuickExampleMissingCaseDirSystemTest : public TestCase
{
  public:
    UbQuickExampleMissingCaseDirSystemTest()
        : TestCase("UnifiedBus - ub-quick-example rejects missing case directory")
    {
    }

    void DoRun() override
    {
        SetDataDir(NS_TEST_SOURCEDIR);
        const std::filesystem::path repoRoot = LocateRepoRoot();
        const std::filesystem::path missingCaseDir = repoRoot / "scratch/ub-case-does-not-exist";
        const std::string expectedError =
            "case path does not exist: " + NormalizeTestPath(missingCaseDir);
        auto [status, output] =
            RunQuickExampleCommand(CreateTempDirFilename(GetName() + ".log"),
                                   "--case-path=\"" + missingCaseDir.string() + "\"",
                                   "",
                                   "");

        NS_TEST_ASSERT_MSG_NE(status,
                              0,
                              "ub-quick-example should fail when case-path directory is missing");
        NS_TEST_ASSERT_MSG_NE(output.find(expectedError),
                              std::string::npos,
                              "ub-quick-example should print a clear missing case directory error");
    }
};

class UbQuickExampleMissingCaseFileSystemTest : public TestCase
{
  public:
    UbQuickExampleMissingCaseFileSystemTest()
        : TestCase("UnifiedBus - ub-quick-example rejects case directory with missing required files")
    {
    }

    void DoRun() override
    {
        SetDataDir(NS_TEST_SOURCEDIR);
        const std::filesystem::path caseDir =
            std::filesystem::path(CreateTempDirFilename("ub-quick-example-missing-files-case"));
        std::filesystem::create_directories(caseDir);
        const std::filesystem::path expectedMissingFile = caseDir / "network_attribute.txt";
        const std::string expectedError =
            "missing required case file: " + NormalizeTestPath(expectedMissingFile);

        auto [status, output] =
            RunQuickExampleCommand(CreateTempDirFilename(GetName() + ".log"),
                                   "--case-path=\"" + caseDir.string() + "\"",
                                   "",
                                   "");

        NS_TEST_ASSERT_MSG_NE(status,
                              0,
                              "ub-quick-example should fail when required case files are missing");
        NS_TEST_ASSERT_MSG_NE(output.find(expectedError),
                              std::string::npos,
                              "ub-quick-example should identify the first missing required case file");
    }
};

class UbQuickExampleHelpTextSystemTest : public TestCase
{
  public:
    UbQuickExampleHelpTextSystemTest()
        : TestCase("UnifiedBus - ub-quick-example help marks case-path as required")
    {
    }

    void DoRun() override
    {
        SetDataDir(NS_TEST_SOURCEDIR);
        auto [status, output] =
            RunQuickExampleCommand(CreateTempDirFilename(GetName() + ".log"), "--help", "");

        NS_TEST_ASSERT_MSG_EQ(status, 0, "ub-quick-example --help should exit successfully");
        NS_TEST_ASSERT_MSG_NE(output.find("Required path to the unified-bus case directory"),
                              std::string::npos,
                              "help text should describe case-path as required");
        NS_TEST_ASSERT_MSG_NE(output.find("Typical usage:"),
                              std::string::npos,
                              "help text should include quick-example usage guidance");
        NS_TEST_ASSERT_MSG_NE(output.find("traffic.csv / UbTrafficGen is single-process only"),
                              std::string::npos,
                              "help text should explain the MPI TrafficGen boundary");
    }
};

class UbQuickExampleLocalSingleThreadSystemTest : public TestCase
{
  public:
    UbQuickExampleLocalSingleThreadSystemTest()
        : TestCase("UnifiedBus - ub-quick-example local mtp-threads=1 runs as single-thread")
    {
    }

    void DoRun() override
    {
        SetDataDir(NS_TEST_SOURCEDIR);
        auto [status, output] =
            RunQuickExampleCommand(CreateTempDirFilename(GetName() + ".log"),
                                   "--mtp-threads=1",
                                   "",
                                   "scratch/2nodes_single-tp");

        NS_TEST_ASSERT_MSG_EQ(status,
                              0,
                              "ub-quick-example local mtp-threads=1 should exit successfully");
        NS_TEST_ASSERT_MSG_EQ(output.find("MPI_Testany() ... before MPI_INIT"), std::string::npos,
                              "ub-quick-example local mtp-threads=1 should not touch MPI before MPI_Init");
    }
};

class UbQuickScratchLegacyAliasSystemTest : public TestCase
{
  public:
    UbQuickScratchLegacyAliasSystemTest()
        : TestCase("UnifiedBus - legacy scratch ub-quick-example remains usable")
    {
    }

    void DoRun() override
    {
        SetDataDir(NS_TEST_SOURCEDIR);
        const std::filesystem::path repoRoot = LocateRepoRoot();
        const std::filesystem::path casePath = repoRoot / "scratch/2nodes_single-tp";
        auto [status, output] =
            RunNs3RunCommand(CreateTempDirFilename(GetName() + ".log"),
                             "scratch/ub-quick-example --case-path=" + casePath.string() +
                                 " --test");

        NS_TEST_ASSERT_MSG_EQ(status, 0, "legacy scratch quick-example should exit successfully");
        NS_TEST_ASSERT_MSG_NE(output.find("TEST : 00000 : PASSED"),
                              std::string::npos,
                              "legacy scratch quick-example should complete the local case");
    }
};

class UbQuickExampleSameCasePathSystemTest : public TestCase
{
  public:
    UbQuickExampleSameCasePathSystemTest()
        : TestCase("UnifiedBus - ub-quick-example accepts equivalent duplicated case-path inputs")
    {
    }

    void DoRun() override
    {
        SetDataDir(NS_TEST_SOURCEDIR);
        const std::filesystem::path repoRoot = LocateRepoRoot();
        const std::filesystem::path sameCasePath =
            (repoRoot / "scratch/2nodes_single-tp/../2nodes_single-tp").lexically_normal();
        auto [status, output] =
            RunQuickExampleCommand(CreateTempDirFilename(GetName() + ".log"),
                                   "\"" + sameCasePath.string() + "\" --stop-ms=1",
                                   "",
                                   "scratch/2nodes_single-tp");

        NS_TEST_ASSERT_MSG_EQ(status,
                              0,
                              "ub-quick-example should accept equivalent duplicated case-path inputs");
        NS_TEST_ASSERT_MSG_EQ(output.find("conflicting case paths provided via --case-path and casePath"),
                              std::string::npos,
                              "equivalent duplicated case-path inputs should not trigger a conflict");
    }
};

class UbQuickExampleConflictingCasePathSystemTest : public TestCase
{
  public:
    UbQuickExampleConflictingCasePathSystemTest()
        : TestCase("UnifiedBus - ub-quick-example rejects conflicting case-path inputs")
    {
    }

    void DoRun() override
    {
        SetDataDir(NS_TEST_SOURCEDIR);
        const std::filesystem::path repoRoot = LocateRepoRoot();
        const std::filesystem::path positionalCasePath = repoRoot / "scratch/ub-mpi-minimal";
        auto [status, output] =
            RunQuickExampleCommand(CreateTempDirFilename(GetName() + ".log"),
                                   "\"" + positionalCasePath.string() + "\"",
                                   "",
                                   "scratch/2nodes_single-tp");

        NS_TEST_ASSERT_MSG_NE(status,
                              0,
                              "ub-quick-example with conflicting case paths should exit with failure");
        NS_TEST_ASSERT_MSG_NE(output.find("conflicting case paths provided via --case-path and casePath"),
                              std::string::npos,
                              "ub-quick-example should print a clear conflicting case-path error");
    }
};

class UbQuickExampleOptionalTransportChannelSystemTest : public TestCase
{
  public:
    UbQuickExampleOptionalTransportChannelSystemTest()
        : TestCase("UnifiedBus - ub-quick-example succeeds without transport_channel.csv")
    {
    }

    void DoRun() override
    {
        SetDataDir(NS_TEST_SOURCEDIR);
        const std::filesystem::path caseDir =
            CopyCaseDirWithoutFile("scratch/2nodes_single-tp", "transport_channel.csv");
        auto [status, output] =
            RunQuickExampleCommand(CreateTempDirFilename(GetName() + ".log"),
                                   "--case-path=\"" + caseDir.string() + "\"",
                                   "",
                                   "");

        std::error_code ec;
        std::filesystem::remove_all(caseDir, ec);

        NS_TEST_ASSERT_MSG_EQ(status,
                              0,
                              "ub-quick-example should accept case directories without transport_channel.csv");
    }
};

class UbQuickExampleLocalDependentDagSingleThreadSystemTest : public TestCase
{
  public:
    UbQuickExampleLocalDependentDagSingleThreadSystemTest()
        : TestCase("UnifiedBus - ub-quick-example local dependent DAG runs in single-thread")
    {
    }

    void DoRun() override
    {
        SetDataDir(NS_TEST_SOURCEDIR);
        const std::string trafficCsv =
            "taskId,sourceNode,destNode,dataSize(Byte),opType,priority,delay,phaseId,dependOnPhases\n"
            "0,0,3,4096,URMA_WRITE,7,10ns,10,\n"
            "1,3,0,4096,URMA_WRITE,7,10ns,20,\n"
            "2,0,3,4096,URMA_WRITE,7,10ns,30,10 20\n";
        const std::filesystem::path caseDir =
            CopyCaseDirWithTrafficFile("scratch/2nodes_single-tp", trafficCsv);

        auto [status, output] =
            RunQuickExampleAbsoluteCaseCommand(CreateTempDirFilename(GetName() + ".log"),
                                               "--mtp-threads=1 --test",
                                               "",
                                               caseDir);

        std::error_code ec;
        std::filesystem::remove_all(caseDir, ec);

        NS_TEST_ASSERT_MSG_EQ(status, 0, "single-thread dependent DAG case should exit successfully");
        NS_TEST_ASSERT_MSG_NE(output.find("TEST : 00000 : PASSED"),
                              std::string::npos,
                              "single-thread dependent DAG case should report PASSED");
    }
};

class UbQuickExampleLocalDependentDagMtpRedSystemTest : public TestCase
{
  public:
    UbQuickExampleLocalDependentDagMtpRedSystemTest()
        : TestCase("UnifiedBus - ub-quick-example local dependent DAG fanout runs in MTP")
    {
    }

    void DoRun() override
    {
        SetDataDir(NS_TEST_SOURCEDIR);
        std::ostringstream traffic;
        traffic << "taskId,sourceNode,destNode,dataSize(Byte),opType,priority,delay,phaseId,"
                   "dependOnPhases\n";
        traffic << "0,0,3,4096,URMA_WRITE,7,10ns,10,\n";
        traffic << "1,3,0,4096,URMA_WRITE,7,10ns,20,\n";
        for (uint32_t taskId = 2; taskId <= 40; ++taskId)
        {
            traffic << taskId << ",0,3,4096,URMA_WRITE,7,10ns," << (100 + taskId) << ",10 20\n";
        }
        const std::filesystem::path caseDir =
            CopyCaseDirWithTrafficFile("scratch/2nodes_single-tp", traffic.str());

        auto [status, output] =
            RunQuickExampleAbsoluteCaseCommand(CreateTempDirFilename(GetName() + ".log"),
                                               "--mtp-threads=2 --test",
                                               "",
                                               caseDir);

        std::error_code ec;
        std::filesystem::remove_all(caseDir, ec);

        NS_TEST_ASSERT_MSG_EQ(status, 0, "MTP dependent DAG fanout case should exit successfully");
        NS_TEST_ASSERT_MSG_NE(output.find("TEST : 00000 : PASSED"),
                              std::string::npos,
                              "MTP dependent DAG fanout case should report PASSED");
    }
};

#ifdef NS3_MPI
class UbQuickExampleMpiCrossRankPhaseDependencySystemTest : public TestCase
{
  public:
    UbQuickExampleMpiCrossRankPhaseDependencySystemTest()
        : TestCase("UnifiedBus - ub-quick-example rejects cross-rank phase dependency under MPI")
    {
    }

    void DoRun() override
    {
        SetDataDir(NS_TEST_SOURCEDIR);
        const std::string trafficCsv =
            "taskId,sourceNode,destNode,dataSize(Byte),opType,priority,delay,phaseId,dependOnPhases\n"
            "0,0,3,4096,URMA_WRITE,7,10ns,10,\n"
            "1,3,0,4096,URMA_WRITE,7,10ns,20,10\n";
        const std::filesystem::path caseDir =
            CopyCaseDirWithTrafficFile("scratch/ub-mpi-minimal", trafficCsv);

        auto [status, output] =
            RunQuickExampleAbsoluteCaseCommand(CreateTempDirFilename(GetName() + ".log"),
                                               "--mtp-threads=2 --stop-ms=1 --test",
                                               "mpirun -np 2",
                                               caseDir);

        std::error_code ec;
        std::filesystem::remove_all(caseDir, ec);

        NS_TEST_ASSERT_MSG_NE(status, 0, "MPI cross-rank dependent DAG command should be rejected");
        NS_TEST_ASSERT_MSG_NE(output.find(UbTrafficGen::GetMultiProcessUnsupportedMessage()),
                              std::string::npos,
                              "cross-rank phase dependency should print the unsupported-runtime message");
    }
};

class UbQuickExampleMpiSystemTest : public TestCase
{
  public:
    UbQuickExampleMpiSystemTest(const std::string& name,
                                const std::string& casePathRelative,
                                const std::string& extraArgs)
        : TestCase(name),
          m_casePathRelative(casePathRelative),
          m_extraArgs(extraArgs)
    {
    }

    void DoRun() override
    {
        SetDataDir(NS_TEST_SOURCEDIR);
        auto [status, output] =
            RunQuickExampleCommand(CreateTempDirFilename(GetName() + ".log"),
                                   m_extraArgs,
                                   "mpirun -np 2",
                                   m_casePathRelative);

        NS_TEST_ASSERT_MSG_NE(status, 0, "ub-quick-example MPI invocation should be rejected");
        NS_TEST_ASSERT_MSG_NE(output.find(UbTrafficGen::GetMultiProcessUnsupportedMessage()),
                              std::string::npos,
                              "MPI quick-example should explain the unsupported UbTrafficGen runtime");
    }

  private:
    std::string m_casePathRelative;
    std::string m_extraArgs;
};
#endif

class UbQuickExampleSystemTestSuite : public TestSuite
{
  public:
    UbQuickExampleSystemTestSuite()
        : TestSuite("unified-bus-examples", Type::SYSTEM)
    {
        AddTestCase(new UbQuickExampleMissingCasePathSystemTest(), TestCase::Duration::QUICK);
        AddTestCase(new UbQuickExampleMissingCaseDirSystemTest(), TestCase::Duration::QUICK);
        AddTestCase(new UbQuickExampleMissingCaseFileSystemTest(), TestCase::Duration::QUICK);
        AddTestCase(new UbQuickExampleHelpTextSystemTest(), TestCase::Duration::QUICK);
        AddTestCase(new UbQuickExampleLocalSingleThreadSystemTest(), TestCase::Duration::QUICK);
        AddTestCase(new UbQuickScratchLegacyAliasSystemTest(), TestCase::Duration::QUICK);
        AddTestCase(new UbQuickExampleSameCasePathSystemTest(), TestCase::Duration::QUICK);
        AddTestCase(new UbQuickExampleConflictingCasePathSystemTest(), TestCase::Duration::QUICK);
        AddTestCase(new UbQuickExampleOptionalTransportChannelSystemTest(),
                    TestCase::Duration::QUICK);
        AddTestCase(new UbQuickExampleLocalDependentDagSingleThreadSystemTest(),
                    TestCase::Duration::QUICK);
        AddTestCase(new UbQuickExampleLocalDependentDagMtpRedSystemTest(),
                    TestCase::Duration::QUICK);
#ifdef NS3_MTP
        AddTestCase(new UbQuickExampleLocalMtpSystemTest(), TestCase::Duration::QUICK);
#endif
#ifdef NS3_MPI
        AddTestCase(new UbQuickExampleSpoofedMpiEnvSystemTest(), TestCase::Duration::QUICK);
        AddTestCase(new UbQuickExampleMpiSystemTest("UnifiedBus - ub-quick-example rejects MPI minimal case",
                                                    "scratch/ub-mpi-minimal",
                                                    ""),
                    TestCase::Duration::QUICK);
        AddTestCase(new UbQuickExampleMpiSystemTest("UnifiedBus - ub-quick-example rejects MPI mtp-threads=1 case",
                                                    "scratch/ub-mpi-minimal",
                                                    "--mtp-threads=1"),
                    TestCase::Duration::QUICK);
        AddTestCase(new UbQuickExampleMpiSystemTest("UnifiedBus - ub-quick-example rejects hybrid minimal case",
                                                    "scratch/ub-mpi-minimal",
                                                    "--mtp-threads=2"),
                    TestCase::Duration::QUICK);
        AddTestCase(new UbQuickExampleMpiSystemTest("UnifiedBus - ub-quick-example rejects hybrid ldst case",
                                                    "scratch/ub-mpi-minimal",
                                                    "--mtp-threads=2"),
                    TestCase::Duration::QUICK);
        AddTestCase(new UbQuickExampleMpiSystemTest("UnifiedBus - ub-quick-example rejects hybrid multi-remote case",
                                                    "scratch/ub-mpi-minimal",
                                                    "--mtp-threads=2"),
                    TestCase::Duration::QUICK);
        AddTestCase(new UbQuickExampleMpiCrossRankPhaseDependencySystemTest(),
                    TestCase::Duration::QUICK);
#endif
    }
};

static UbQuickExampleSystemTestSuite g_ubQuickExampleSystemTestSuite;

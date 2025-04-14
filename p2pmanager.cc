#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/internet-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/applications-module.h"
#include "ns3/mobility-module.h"
#include "ns3/config-store-module.h"
#include "ns3/random-variable-stream.h"
#include "ns3/log.h"
#include "p2pmanager.h"


#include <vector>
#include <map>
#include <string>
#include <iostream>
#include <memory>

NS_LOG_COMPONENT_DEFINE("P2PManager");

using namespace ns3;

P2PManager::P2PManager(uint32_t numNodes, double meanLatency, double latencyVariance,
                       double shareGenMean, double shareGenVariance,
                       uint32_t maxTipsToReference, uint32_t simulationDuration)
    : m_numNodes(numNodes),
      m_meanLatency(meanLatency),
      m_latencyVariance(latencyVariance),
      m_shareGenMean(shareGenMean),
      m_shareGenVariance(shareGenVariance),
      m_maxTipsToReference(maxTipsToReference),
      m_simulationDuration(simulationDuration) {
    NS_LOG_FUNCTION(this << numNodes << meanLatency << latencyVariance << shareGenMean
                         << shareGenVariance << maxTipsToReference << simulationDuration);
    LogComponentEnable("P2PManager", LOG_LEVEL_INFO);
}

void P2PManager::Configure() {
    NS_LOG_FUNCTION(this);

    m_nodes.Create(m_numNodes);
    
    InternetStackHelper internet;
    internet.Install(m_nodes);
    
    PointToPointHelper p2p;
    p2p.SetDeviceAttribute("DataRate", StringValue("5Mbps"));
    p2p.SetChannelAttribute("Delay", StringValue("2ms"));
    
    uint32_t subnetCount = 1;
    std::map<uint32_t, std::vector<Ipv4Address>> nodeToAddresses;
    
    for (uint32_t i = 0; i < m_numNodes; ++i) {
        for (uint32_t j = i + 1; j < m_numNodes; ++j) {
            NodeContainer nodeContainer;
            nodeContainer.Add(m_nodes.Get(i));
            nodeContainer.Add(m_nodes.Get(j));

            NetDeviceContainer link = p2p.Install(nodeContainer);
            
            Ipv4AddressHelper ipv4;
            std::ostringstream subnet;
            uint32_t firstOctet = 10;
            uint32_t secondOctet = (subnetCount / 256) % 256; 
            uint32_t thirdOctet  = subnetCount % 256;           
            subnet << firstOctet << "." << secondOctet << "." << thirdOctet << ".0";
            ipv4.SetBase(subnet.str().c_str(), "255.255.255.0");
            
            Ipv4InterfaceContainer interfaces = ipv4.Assign(link);
            
            uint32_t nodeId1 = m_nodes.Get(i)->GetId();
            nodeToAddresses[nodeId1].push_back(interfaces.GetAddress(0));
            
            uint32_t nodeId2 = m_nodes.Get(j)->GetId();
            nodeToAddresses[nodeId2].push_back(interfaces.GetAddress(1));
            subnetCount++;
        }
    }
    
    Ipv4GlobalRoutingHelper::PopulateRoutingTables();
    time_t now = time(nullptr);
    time_t maxTime = now + m_simulationDuration / 4;
    
    for (uint32_t i = 0; i < m_numNodes; ++i) {
        Ptr<RandomVariableStream> latencyModel = CreateLatencyModel(i);
        Ptr<NormalRandomVariable> shareGenModel = CreateShareGenTimeModel(i);
        Ptr<P2PoolNode> p2pNode = Create<P2PoolNode>(i, shareGenModel, latencyModel,
                                                     m_maxTipsToReference, maxTime);
        m_nodes.Get(i)->AddApplication(p2pNode);
        p2pNode->SetStartTime(Seconds(0.0));
        p2pNode->SetStopTime(Seconds(m_simulationDuration + 1.0));
        m_p2pNodes.push_back(p2pNode);
    }
    
    Simulator::Schedule(Seconds(5) + Simulator::Now(), &P2PManager::SetupAllConnections, this, nodeToAddresses);
    
    NS_LOG_INFO("Network configured with " << m_numNodes << " nodes");
}


void P2PManager::SetupAllConnections(std::map<uint32_t, std::vector<Ipv4Address>> &nodeToAddresses) {
    NS_LOG_INFO("Setting up all connections at " << Simulator::Now().GetSeconds() << "s");
    for (uint32_t i = 0; i < m_numNodes; ++i) {
        m_p2pNodes[i]->SetupConnections(m_p2pNodes, nodeToAddresses, 9333);

}
}

void P2PManager::Run() {
    NS_LOG_FUNCTION(this);

    NS_LOG_INFO("Starting simulation for " << m_simulationDuration << " seconds");
    Simulator::Schedule(Seconds(100), [](){
        NS_LOG_INFO("Dummy event reached at 1000 seconds.");
    });
    Simulator::Stop(Seconds(m_simulationDuration));
    Simulator::Run();
    Simulator::Destroy();
    NS_LOG_INFO("Simulation completed");
}

void P2PManager::PrintResults() {
    NS_LOG_FUNCTION(this);

    std::cout << "=== P2Pool Simulation Results ===" << std::endl;
    uint32_t totalOrphans = 0;

    for (uint32_t i = 0; i < m_numNodes; ++i) {
        m_p2pNodes[i]->PrintChainStats();
        totalOrphans += m_p2pNodes[i]->getOrphanCount();
    }

    std::cout << "Average orphans per node: " << (double)totalOrphans / m_numNodes << std::endl;
}

Ptr<RandomVariableStream> P2PManager::CreateLatencyModel(uint32_t nodeId) {
    double nodeFactor = 0.8 + (nodeId % 5) * 0.1;
    double mean = m_meanLatency * nodeFactor;
    double variance = m_latencyVariance * nodeFactor;

    Ptr<NormalRandomVariable> latencyModel = CreateObject<NormalRandomVariable>();
    latencyModel->SetAttribute("Mean", DoubleValue(mean));
    latencyModel->SetAttribute("Variance", DoubleValue(variance));

    NS_LOG_INFO("Created latency model for node " << nodeId << " with mean="
                << mean << ", variance=" << variance);
    return latencyModel;
}

Ptr<NormalRandomVariable> P2PManager::CreateShareGenTimeModel(uint32_t nodeId) {
    double hashPowerFactor = 0.5 + ((nodeId * 7919) % 100) / 100.0;
    double mean = m_shareGenMean / hashPowerFactor;
    double variance = m_shareGenVariance / hashPowerFactor;

    Ptr<NormalRandomVariable> shareGenModel = CreateObject<NormalRandomVariable>();
    shareGenModel->SetAttribute("Mean", DoubleValue(mean));
    shareGenModel->SetAttribute("Variance", DoubleValue(variance));

    NS_LOG_INFO("Created share generation model for node " << nodeId << " with mean="
                << mean << ", variance=" << variance << " (hash power factor: "
                << hashPowerFactor << ")");
    return shareGenModel;
}
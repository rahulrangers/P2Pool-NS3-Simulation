#include "p2pmanager.h"

#include "ns3/applications-module.h"
#include "ns3/config-store-module.h"
#include "ns3/core-module.h"
#include "ns3/internet-module.h"
#include "ns3/log.h"
#include "ns3/mobility-module.h"
#include "ns3/network-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/random-variable-stream.h"

#include <cmath>
#include <iostream>
#include <map>
#include <memory>
#include <random>
#include <string>
#include <vector>

NS_LOG_COMPONENT_DEFINE("P2PManager");

using namespace ns3;

P2PManager::P2PManager(uint32_t numNodes,
                       double shareGenMean,
                       double shareGenVariance,
                       uint32_t maxTipsToReference,
                       uint32_t simulationDuration,
                       Time maxTimeStamp)
    : numNodes(numNodes),
      shareGenMean(shareGenMean),
      shareGenVariance(shareGenVariance),
      maxTipsToReference(maxTipsToReference),
      simulationDuration(simulationDuration),
      maxTime(maxTimeStamp)
{
    nodes.Create(numNodes);
    internet.Install(nodes);
    addressHelper.SetBase("10.1.0.0", "255.255.255.0");
    NS_LOG_FUNCTION(this << numNodes << shareGenMean << shareGenVariance << maxTipsToReference
                         << simulationDuration);
    LogComponentEnable("P2PManager", LOG_LEVEL_INFO);
}

    
void P2PManager::CreateRandomTopology(double connectionProbability, double latency)
    {
        NS_LOG_FUNCTION(this);
        std::random_device rd;
        std::mt19937 rng(rd());
        std::uniform_real_distribution<double> dist(0.0, 1.0);
        for (uint32_t i = 0; i < numNodes; i++)
        {
            bool connected = false;
            for (uint32_t j = i + 1; j < numNodes; j++)
            {
                if (dist(rng) < connectionProbability)
                {
                    connected = true;
                    ConnectNodes(i, j, latency);
                }
            }

            if (!connected)
            {
                if (i == 0 && numNodes > 0)
                {
                    ConnectNodes(0, 1, latency);
                }
                else
                {
                    ConnectNodes(i, i - 1, latency);
                }
            }
        }

        for (uint32_t i = 0; i < numNodes; ++i)
        {
            Ptr<NormalRandomVariable> shareGenModel = CreateShareGenTimeModel(i);
            Ptr<P2PoolNode> p2pNode = Create<P2PoolNode>(i, shareGenModel, maxTipsToReference, maxTime);
            nodes.Get(i)->AddApplication(p2pNode);
            p2pNode->SetStartTime(Seconds(0.0));
            p2pNode->SetStopTime(Seconds(simulationDuration + 1.0));
            p2pNodes.push_back(p2pNode);
        }
        Ipv4GlobalRoutingHelper::PopulateRoutingTables();

        Simulator::Schedule(Seconds(5) + Simulator::Now(), &P2PManager::makeconnections, this);

        NS_LOG_INFO("Network configured with " << latency << " lactency");
    }

    
void P2PManager::makeconnections()
    {
        for (const auto& connection : connections)
        {
            uint32_t i = connection.first.first;
            uint32_t j = connection.first.second;
            ConnectPeerSockets(i, j);
        }
        startGeneratingShares();
    }

   
void P2PManager::startGeneratingShares()
    {
        for (auto& node : p2pNodes)
        {
            node->ScheduleNextShareGeneration();
        }
    }

    
void P2PManager::ConnectNodes(uint32_t i, uint32_t j, double latencyMs)
    {
        PointToPointHelper p2pHelper;
        p2pHelper.SetDeviceAttribute("DataRate", StringValue("5Mbps"));
        p2pHelper.SetChannelAttribute("Delay", TimeValue(MilliSeconds(latencyMs)));

        NodeContainer linkNodes;
        linkNodes.Add(nodes.Get(i));
        linkNodes.Add(nodes.Get(j));

        NetDeviceContainer linkDevices = p2pHelper.Install(linkNodes);

        Ipv4InterfaceContainer ifc = addressHelper.Assign(linkDevices);
        addressHelper.NewNetwork();

        ConnectionInfo connInfo;
        connInfo.devices = linkDevices;
        connInfo.channel = linkDevices.Get(0)->GetChannel()->GetObject<PointToPointChannel>();
        connInfo.ifc = ifc;

        connections[std::make_pair(i, j)] = connInfo;
    }

    
void P2PManager::ConnectPeerSockets(uint32_t i, uint32_t j)
    {
        auto& conn = connections[{i, j}];
        Ipv4Address addrJ = conn.ifc.GetAddress(1);

        Ptr<Socket> socket = Socket::CreateSocket(nodes.Get(i), TcpSocketFactory::GetTypeId());

        socket->Connect(InetSocketAddress(addrJ, j + 1000));

        socket->SetRecvCallback(MakeCallback(&P2PoolNode::HandleReceivedShare, p2pNodes[i]));
        NS_LOG_INFO("connection " << i << ' ' << j << ' ' << addrJ);
        p2pNodes[i]->AddPeerSocket(j, socket);
        std::string reg = "REGISTER:" + std::to_string(i);
        Ptr<Packet> packet =
            Create<Packet>(reinterpret_cast<const uint8_t*>(reg.c_str()), reg.length() + 1);
        socket->Send(packet);
    }

    
void P2PManager::Run()
    {
        NS_LOG_FUNCTION(this);

        NS_LOG_INFO("Starting simulation for " << simulationDuration << " seconds");
        Simulator::Stop(Seconds(simulationDuration));
        Simulator::Run();
        Simulator::Destroy();
        NS_LOG_INFO("Simulation completed");
    }

    
void P2PManager::PrintResults()
    {
        NS_LOG_FUNCTION(this);

        std::cout << "=== P2Pool Simulation Results ===" << std::endl;
        uint32_t totalOrphans = 0;

        for (uint32_t i = 0; i < numNodes; ++i)
        {
            p2pNodes[i]->PrintChainStats();
            totalOrphans += p2pNodes[i]->getOrphanCount();
        }

        std::cout << "Average orphans per node: " << (double)totalOrphans / numNodes << std::endl;
    }

   
Ptr<NormalRandomVariable> P2PManager::CreateShareGenTimeModel(uint32_t nodeId)
    {
        double hashPowerFactor = 0.5 + ((nodeId * 7919) % 100) / 100.0;
        double mean = shareGenMean / hashPowerFactor;
        double variance = shareGenVariance / hashPowerFactor;

        Ptr<NormalRandomVariable> shareGenModel = CreateObject<NormalRandomVariable>();
        shareGenModel->SetAttribute("Mean", DoubleValue(mean));
        shareGenModel->SetAttribute("Variance", DoubleValue(variance));

        NS_LOG_INFO("Created share generation model for node "
                    << nodeId << " with mean=" << mean << ", variance=" << variance
                    << " (hash power factor: " << hashPowerFactor << ")");
        return shareGenModel;
    }

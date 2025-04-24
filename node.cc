#include "node.h"

#include "ns3/applications-module.h"
#include "ns3/core-module.h"
#include "ns3/internet-module.h"
#include "ns3/log.h"
#include "ns3/network-module.h"
#include "ns3/packet.h"
#include "ns3/point-to-point-module.h"
#include "ns3/random-variable-stream.h"
#include "ns3/simulator.h"
#include "ns3/socket.h"
#include "ns3/tcp-socket.h"

#include <algorithm>
#include <ctime>
#include <functional>
#include <iostream>
#include <map>
#include <memory>
#include <sstream>
#include <string>
#include <vector>

using namespace ns3;

NS_LOG_COMPONENT_DEFINE("P2PoolNode");

P2PoolNode::P2PoolNode(uint32_t nodeId,
                       Ptr<NormalRandomVariable> shareGenTimeModel,
                       uint32_t maxTipsToReference,
                       ns3::Time max_share_time)
    : nodeId(nodeId),
      shareChain(new ShareChain(max_share_time)),
      maxTipsToReference(maxTipsToReference),
      shareGenTimeModel(shareGenTimeModel),
      running(false),
      sharesCreated(0),
      sharesReceived(0),
      sharesSent(0),
      maxTime(max_share_time)
{
    LogComponentEnable("P2PoolNode", LOG_LEVEL_INFO);
}

P2PoolNode::~P2PoolNode()
{
    delete shareChain;
}

void P2PoolNode::StartApplication(void)
{
    running = true;
    if (!socket)
    {
        socket = Socket::CreateSocket(GetNode(), TcpSocketFactory::GetTypeId());
        InetSocketAddress local = InetSocketAddress(Ipv4Address::GetAny(), nodeId+1000);
        int result = socket->Bind(local);
        if (result != 0)
        {
            NS_LOG_ERROR("Node " << nodeId << " failed to bind to port, error: " << result);
            return;
        }
        socket->Listen();
       
        socket->SetAcceptCallback(MakeCallback(&P2PoolNode::ConnectionRequestCallback, this),
                                    MakeCallback(&P2PoolNode::ConnectionAcceptedCallback, this));
    }
}

void P2PoolNode::StopApplication(void)
{
    running = false;

    if (nextShareEvent.IsRunning())
    {
        Simulator::Cancel(nextShareEvent);
    }

    if (socket)
    {
        socket->Close();
        socket->SetRecvCallback(MakeNullCallback<void, Ptr<Socket>>());
    }

    for (auto& peer : peerSockets)
    {
        peer.second->Close();
    }
    peerSockets.clear();
}


uint32_t P2PoolNode::GetNodeId() const
{
    return nodeId;
}

ShareChain* P2PoolNode::GetShareChain() const
{
    return shareChain;
}

void P2PoolNode::StopShareGeneration()
{
    if (nextShareEvent.IsRunning())
    {
        Simulator::Cancel(nextShareEvent);
    }
}

uint32_t P2PoolNode::getOrphanCount() const
{
    return shareChain->getOrphanCount();
}

void P2PoolNode::PrintChainStats() const
{

    std::cout << "Node " << nodeId << " statistics:" << std::endl;
    std::cout << "  - Shares created: " << sharesCreated << std::endl;
    std::cout << "  - Shares received: " << sharesReceived << std::endl;
    std::cout << "  - Shares sent: " << sharesSent << std::endl;
    std::cout << "  - Orphan count: " << shareChain->getOrphanCount() << std::endl;
    std::cout << "  - Total Shares: " << shareChain->getTotalShares() << std::endl;
    std::cout << "  - Uncle BLocks " << shareChain->getUncleBlocks() << std::endl;
    std::cout << "  - MAin chainlen: " << shareChain->MainChainLength() << std::endl;
    std::vector<uint32_t> a = shareChain->showchain();
    for(int i=0;i<a.size();i++){
        std::cout<<a[i]<<' ';
    }
    std::cout<<std::endl;
}

void P2PoolNode::GenerateAndBroadcastShare()
{
    NS_LOG_FUNCTION("hey am i beinge generated is the issue oot ere debugging");

    std::unordered_map<uint32_t, uint32_t> tips = shareChain->getChainTips();
    std::vector<std::pair<uint32_t, int>> sortedtips(tips.begin(), tips.end());
    std::vector<uint32_t> tipShares;
    
    std::sort(sortedtips.begin(), sortedtips.end(), [](const auto& a, const auto& b) {
        return a.second > b.second;
    });
    uint32_t noOfTips = std::min(static_cast<uint32_t>(tips.size()), maxTipsToReference);
    for (int i = 0; i < noOfTips; i++)
    {
        tipShares.push_back(sortedtips[i].first);
    }
    ns3::Time now = Simulator::Now();
    ns3::Time nowInSeconds = Seconds(now.GetSeconds());
    uint32_t uniqueshareid = GenerateUniqueShareId();
    
    int dir_status = system("mkdir -p output");
    std::string filename = "output/node_" + std::to_string(nodeId) + "_shares.csv";
    std::ofstream outFile(filename.c_str(), std::ios::app);
    if (outFile.is_open()) {
        outFile << uniqueshareid << "," << nowInSeconds.GetSeconds() <<','<< tipShares.size()<<", "<<sortedtips[0].first<<std::endl;
        outFile.flush();  
        outFile.close();
    } else {
        NS_LOG_ERROR("Node " << nodeId << " failed to open file " << filename);
    }

    Share* newShare = new Share(uniqueshareid, nodeId, nowInSeconds, tipShares,sortedtips[0].first);

    shareChain->addShare(newShare);
    sharesCreated++;
    BroadcastShare(newShare);
    ScheduleNextShareGeneration();
}

uint32_t P2PoolNode::GenerateUniqueShareId()
{
    uint64_t timestamp = static_cast<uint64_t>(Simulator::Now().GetTimeStep());
    uint64_t nodePrefix = static_cast<uint64_t>(nodeId) << 48;
    uint64_t shareCount = static_cast<uint64_t>(sharesCreated) << 32;
    uint64_t seed = nodePrefix | shareCount | (timestamp & 0xFFFFFFFF);
    std::hash<uint64_t> hasher;
    return static_cast<uint32_t>(hasher(seed));
}

void P2PoolNode::ScheduleNextShareGeneration()
{
    double nextTime = std::max(0.1, shareGenTimeModel->GetValue());

    nextShareEvent = Simulator::Schedule(Seconds(nextTime),
                                           &P2PoolNode::GenerateAndBroadcastShare,
                                           this);
}

void P2PoolNode::BroadcastShare(Share* share)
{
    sharesSent++;
    for (auto& peer : peerSockets)
    {
        SendShareToPeer(share,peer.second);
    }
}

void P2PoolNode::SendShareToPeer(Share* share, Ptr<Socket> socket)
{
    std::string serializedShare = SerializeShare(share);
    Ptr<Packet> packet =
        Create<Packet>((uint8_t*)serializedShare.c_str(), serializedShare.size() + 1);
    socket->Send(packet);
}

void P2PoolNode::HandleReceivedShare(Ptr<Socket> socket)
{

    Ptr<Packet> packet;
    Address from;

    while ((packet = socket->RecvFrom(from)))
    {
        uint32_t size = packet->GetSize();
        uint8_t* buffer = new uint8_t[size];
        packet->CopyData(buffer, size);
        std::string data(reinterpret_cast<char*>(buffer), size);
        delete[] buffer;
        if (data.find("REGISTER:") == 0)
        {
            size_t colonPos = data.find(":");
            if (colonPos != std::string::npos)
            {
                uint32_t peerId = std::stoul(data.substr(colonPos + 1));
                NS_LOG_INFO("Node " << nodeId << " received registration from peer " << peerId);
                peerSockets[peerId] = socket;
            }
        }
        else
        {
        Share* receivedShare = DeserializeShare(data);
        if (existingShares.find(receivedShare->getShareId()) != existingShares.end())
        {
            NS_LOG_INFO("Node " << nodeId << " already processed share " << receivedShare->getShareId() << ":"
                                << receivedShare->getShareId());
        }
        else if (receivedShare)
        {
            shareChain->addShare(receivedShare);
            BroadcastShare(receivedShare);
            existingShares.insert(receivedShare->getShareId());
            sharesReceived++;
        }
     }
    }
}

bool P2PoolNode::ConnectionRequestCallback(Ptr<Socket> socket, const Address& address)
{
    return true;
}

void P2PoolNode::ConnectionAcceptedCallback(Ptr<Socket> socket, const Address& address)
{
    socket->SetRecvCallback(MakeCallback(&P2PoolNode::HandleReceivedShare, this));
}

std::string P2PoolNode::SerializeShare(Share* share)
{
    if (!share)
    {
        return "";
    }

    std::stringstream ss;
    ss << share->getShareId() << "|";
    ss << share->getSenderId() << "|";
    ss << share->getTimestamp().GetSeconds() << "|";
    ss << share->getParentId()<< "|";
    
    const std::vector<uint32_t>& prevRefs = share->getPrevRefs();
    ss << prevRefs.size() << "|";

    for (size_t i = 0; i < prevRefs.size(); ++i)
    {
        ss << prevRefs[i];
        if (i < prevRefs.size() - 1)
        {
            ss << ",";
        }
    }
    return ss.str();
}

void P2PoolNode::AddPeerSocket(uint32_t peerId, Ptr<Socket> socket)
{
    peerSockets[peerId] = socket;
    NS_LOG_INFO("Node " << nodeId << " added socket connection to peer " << peerId);
}

Share* P2PoolNode::DeserializeShare(const std::string& data)
{
    std::stringstream ss(data);
    std::string token;
    std::vector<std::string> tokens;
    while (std::getline(ss, token, '|'))
    {
        tokens.push_back(token);
    }
    if (tokens.size() < 5)
    {
        return nullptr;
    }

    try
    {
        uint32_t shareId = std::stoul(tokens[0]);
        uint32_t senderId = std::stoul(tokens[1]);
        ns3::Time timestamp = Seconds(std::stoll(tokens[2]));
        uint32_t parentId = std::stoll(tokens[3]);
        uint32_t numRefs = std::stoul(tokens[4]);

        std::vector<uint32_t> prevRefs;

        if (tokens.size() > 5 && numRefs > 0)
        {
            std::stringstream refSs(tokens[5]);
            std::string refToken;

            while (std::getline(refSs, refToken, ','))
            {
                if (!refToken.empty())
                {
                    uint32_t refShareId = std::stoul(refToken);
                    prevRefs.push_back(refShareId);
                }
            }
        }
        return new Share(shareId, senderId, timestamp, prevRefs,parentId);
    }
    catch (const std::exception& e)
    {
        return nullptr;
    }
}

NS_OBJECT_ENSURE_REGISTERED(P2PoolNode);

TypeId P2PoolNode::GetTypeId(void)
{
    static TypeId tid =
        TypeId("ns3::P2Pool::P2PoolNode").SetParent<Application>().SetGroupName("P2Pool");
    return tid;
}

void P2PoolNode::ConnectionSucceeded(Ptr<Socket> socket)
{
    Address peerAddress;
    socket->GetPeerName(peerAddress);
}


void P2PoolNode::ConnectionFailed(Ptr<Socket> socket)
{
    Address peerAddress;
    socket->GetPeerName(peerAddress);
    NS_LOG_WARN("Node " << nodeId << " failed to connect to " << peerAddress);
}

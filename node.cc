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
                       Ptr<RandomVariableStream> latencyModel,
                       uint32_t maxTipsToReference,
                       time_t max_share_time)
    : m_nodeId(nodeId),
      m_shareChain(new ShareChain(max_share_time)),
      m_maxTipsToReference(maxTipsToReference),
      m_shareGenTimeModel(shareGenTimeModel),
      m_latencyModel(latencyModel),
      m_running(false),
      m_nextShareId(0),
      m_sharesCreated(0),
      m_sharesReceived(0),
      m_sharesSent(0),
      maxTime(max_share_time)
{
    NS_LOG_FUNCTION(this << nodeId);
    LogComponentEnable("P2PoolNode", LOG_LEVEL_INFO);
}

P2PoolNode::~P2PoolNode()
{
    NS_LOG_FUNCTION(this);
    delete m_shareChain;
}

void P2PoolNode::StartApplication(void)
{
    NS_LOG_FUNCTION(this);
    m_running = true;
    NS_LOG_INFO("Node " << m_nodeId << " starting application");
    if (!m_socket)
    {
        NS_LOG_INFO("Node " << m_nodeId << " creating socket");
        m_socket = Socket::CreateSocket(GetNode(), TcpSocketFactory::GetTypeId());
        InetSocketAddress local = InetSocketAddress(Ipv4Address::GetAny(), 9333);
        int result = m_socket->Bind(local);
        if (result != 0)
        {
            NS_LOG_ERROR("Node " << m_nodeId << " failed to bind to port 9333, error: " << result);
            return;
        }
        m_socket->Listen();
        NS_LOG_INFO("Node " << m_nodeId << " listening on port 9333");
        m_socket->SetAcceptCallback(MakeCallback(&P2PoolNode::ConnectionRequestCallback, this),
                                    MakeCallback(&P2PoolNode::ConnectionAcceptedCallback, this));
    }
}

void P2PoolNode::StopApplication(void)
{
    NS_LOG_FUNCTION(this);
    m_running = false;

    if (m_nextShareEvent.IsRunning())
    {
        Simulator::Cancel(m_nextShareEvent);
    }

    if (m_socket)
    {
        m_socket->Close();
        m_socket->SetRecvCallback(MakeNullCallback<void, Ptr<Socket>>());
    }

    for (auto& peer : m_peerSockets)
    {
        peer.second->Close();
    }
    m_peerSockets.clear();
}

void P2PoolNode::SetupConnections(const std::vector<Ptr<P2PoolNode>>& peers,
                             std::map<uint32_t, std::vector<Ipv4Address>>& nodeToAddresses,
                             uint16_t port)
{
    NS_LOG_FUNCTION(this);

    Simulator::Schedule(Simulator::Now() + Seconds(2),
                        &P2PoolNode::DoSetupConnections,
                        this,
                        peers,
                        nodeToAddresses,
                        port);
}

void P2PoolNode::DoSetupConnections(const std::vector<Ptr<P2PoolNode>>& peers,
                               std::map<uint32_t, std::vector<Ipv4Address>>& nodeToAddresses,
                               uint16_t port)
{
    NS_LOG_INFO("Node " << m_nodeId << " setting up connections at time "
                        << Simulator::Now().GetSeconds());

    for (size_t i = 0; i < peers.size(); ++i)
    {
        Ptr<P2PoolNode> peerNode = peers[i];

        if (peerNode->GetNodeId() <= m_nodeId)
        {
            continue;
        }

        auto it = nodeToAddresses.find(peerNode->GetNodeId());
        if (it == nodeToAddresses.end() || it->second.empty())
        {
            NS_LOG_WARN("No IP address found for peer node " << peerNode->GetNodeId());
            continue;
        }

        Ipv4Address peerIp = it->second.front();

        TypeId tid = TcpSocketFactory::GetTypeId();
        Ptr<Socket> socket = Socket::CreateSocket(GetNode(), tid);

        socket->SetRecvCallback(MakeCallback(&P2PoolNode::HandleReceivedShare, this));

        socket->SetConnectCallback(MakeCallback(&P2PoolNode::ConnectionSucceeded, this),
                                   MakeCallback(&P2PoolNode::ConnectionFailed, this));

        InetSocketAddress peerAddress(peerIp, port);
        int result = socket->Connect(peerAddress);

        if (result == 0)
        {
            m_peerSockets[peerAddress] = socket;
        }
        else
        {
            NS_LOG_WARN("Node " << m_nodeId << " failed to connect to Node "
                                << peerNode->GetNodeId() << ", error code: " << result);
        }
    }

    double firstShareDelay = std::max(0.1, m_shareGenTimeModel->GetValue());
    Simulator::Schedule(Simulator::Now() + Seconds(firstShareDelay),
                        &P2PoolNode::GenerateAndBroadcastShare,
                        this);
}

uint32_t P2PoolNode::GetNodeId() const
{
    return m_nodeId;
}

ShareChain* P2PoolNode::GetShareChain() const
{
    return m_shareChain;
}

void P2PoolNode::StartShareGeneration(double interval)
{
    NS_LOG_FUNCTION(this << interval);
    if (!m_running)
    {
        return;
    }
    double delay = std::max(0.1, m_shareGenTimeModel->GetValue());

    m_nextShareEvent = Simulator::Schedule(Simulator::Now() + Seconds(delay),
                                           &P2PoolNode::GenerateAndBroadcastShare,
                                           this);
  //  NS_LOG_INFO("Node " << m_nodeId << " scheduled first share in " << delay << " seconds");
}

void P2PoolNode::StopShareGeneration()
{
    NS_LOG_FUNCTION(this);
    if (m_nextShareEvent.IsRunning())
    {
        Simulator::Cancel(m_nextShareEvent);
    }
}

uint32_t P2PoolNode::getOrphanCount() const
{
    return m_shareChain->getOrphanCount();
}

void P2PoolNode::PrintChainStats() const
{
    NS_LOG_FUNCTION(this);

    std::cout << "Node " << m_nodeId << " statistics:" << std::endl;
    std::cout << "  - Shares created: " << m_sharesCreated << std::endl;
    std::cout << "  - Shares received: " << m_sharesReceived << std::endl;
    std::cout << "  - Shares sent: " << m_sharesSent << std::endl;
    std::cout << "  - Orphan count: " << m_shareChain->getOrphanCount() << std::endl;
    std::cout << "  - Total Shares: " << m_shareChain->getTotalShares() << std::endl;
}

void P2PoolNode::GenerateAndBroadcastShare()
{
    NS_LOG_FUNCTION(this);

    std::unordered_map<uint32_t, uint32_t> tips = m_shareChain->getChainTips();
    std::vector<std::pair<uint32_t, int>> sortedtips(tips.begin(), tips.end());
    std::vector<uint32_t> tipShares;
    
    std::sort(sortedtips.begin(), sortedtips.end(), [](const auto& a, const auto& b) {
        return a.second > b.second;
    });
    uint32_t noOfTips = std::min(static_cast<uint32_t>(tips.size()), m_maxTipsToReference);
    for (int i = 0; i < noOfTips; i++)
    {
        tipShares.push_back(sortedtips[i].first);
    }
    time_t now = std::time(nullptr);
    uint32_t uniqueshareid = GenerateUniqueShareId();
    Share* newShare = new Share(uniqueshareid, m_nodeId, now, tipShares,sortedtips[0].first);
    NS_LOG_INFO("noOfTips "<<sortedtips.size());
    m_shareChain->addShare(newShare);
    m_sharesCreated++;
    NS_LOG_INFO("Node " << m_nodeId << " created share " << newShare->getShareId());
    BroadcastShare(newShare);
    ScheduleNextShareGeneration();
}

uint32_t P2PoolNode::GenerateUniqueShareId()
{
    uint64_t timestamp = static_cast<uint64_t>(Simulator::Now().GetTimeStep());
    uint64_t nodePrefix = static_cast<uint64_t>(m_nodeId) << 48;
    uint64_t shareCount = static_cast<uint64_t>(m_sharesCreated) << 32;
    uint64_t seed = nodePrefix | shareCount | (timestamp & 0xFFFFFFFF);
    std::hash<uint64_t> hasher;
    return static_cast<uint32_t>(hasher(seed));
}

void P2PoolNode::ScheduleNextShareGeneration()
{
    double nextTime = std::max(0.1, m_shareGenTimeModel->GetValue());

    m_nextShareEvent = Simulator::Schedule(Simulator::Now() + Seconds(nextTime),
                                           &P2PoolNode::GenerateAndBroadcastShare,
                                           this);
    // NS_LOG_INFO("Node " << m_nodeId << " scheduled next share in " << nextTime
    //                     << " seconds "
    //                        "currentime "
    //                     << Simulator::Now().GetSeconds());
}

void P2PoolNode::BroadcastShare(Share* share)
{
    NS_LOG_FUNCTION(this << share->getShareId());
    m_sharesSent++;
    for (auto& peer : m_peerSockets)
    {
        double delay = std::max(0.1, m_latencyModel->GetValue());
        Simulator::Schedule(Simulator::Now() + Seconds(delay),
                            &P2PoolNode::SendShareToPeer,
                            this,
                            share,
                            peer.first);
        // NS_LOG_INFO("Node " << m_nodeId << " scheduled share " << share->getShareId()
        //                     << " to be sent in " << delay << " seconds");
    }
}

void P2PoolNode::SendShareToPeer(Share* share, Address peerAddress)
{
    NS_LOG_FUNCTION(this << share->getShareId());

    auto it = m_peerSockets.find(peerAddress);
    if (it == m_peerSockets.end())
    {
        NS_LOG_WARN("Node " << m_nodeId << " can't find socket for peer");
        return;
    }
    std::string serializedShare = SerializeShare(share);
    Ptr<Packet> packet =
        Create<Packet>((uint8_t*)serializedShare.c_str(), serializedShare.size() + 1);
    it->second->Send(packet);

    // NS_LOG_INFO("Node " << m_nodeId << " sent share " << share->getShareId() << " to peer at "
    //                     << peerAddress);
}

void P2PoolNode::HandleReceivedShare(Ptr<Socket> socket)
{
    NS_LOG_FUNCTION(this);

    Ptr<Packet> packet;
    Address from;

    while ((packet = socket->RecvFrom(from)))
    {
        uint32_t size = packet->GetSize();
        uint8_t* buffer = new uint8_t[size];
        packet->CopyData(buffer, size);
        std::string data(reinterpret_cast<char*>(buffer), size);
        delete[] buffer;
        Share* receivedShare = DeserializeShare(data);
        if (receivedShare)
        {
            m_shareChain->addShare(receivedShare);
            m_sharesReceived++;

            // NS_LOG_INFO("Node " << m_nodeId << " received share " << receivedShare->getShareId()
            //                     << " from node " << receivedShare->getSenderId());
        }
    }
}

bool P2PoolNode::ConnectionRequestCallback(Ptr<Socket> socket, const Address& address)
{
    NS_LOG_FUNCTION(this << socket << address);
    return true;
}

void P2PoolNode::ConnectionAcceptedCallback(Ptr<Socket> socket, const Address& address)
{
    NS_LOG_FUNCTION(this << socket << address);

    socket->SetRecvCallback(MakeCallback(&P2PoolNode::HandleReceivedShare, this));

    InetSocketAddress inetAddr = InetSocketAddress::ConvertFrom(address);
    m_peerSockets[address] = socket;
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
    ss << share->getTimestamp() << "|";
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
        time_t timestamp = std::stoll(tokens[2]);
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
    NS_LOG_INFO("Node " << m_nodeId << " successfully connected to " << peerAddress);
}


void P2PoolNode::ConnectionFailed(Ptr<Socket> socket)
{
    Address peerAddress;
    socket->GetPeerName(peerAddress);
    NS_LOG_WARN("Node " << m_nodeId << " failed to connect to " << peerAddress);
}

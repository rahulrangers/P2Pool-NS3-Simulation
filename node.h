#include "share.h"
#include "sharechain.h"

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

#include <ctime>
#include <iostream>
#include <map>
#include <memory>
#include <sstream>
#include <string>
#include <vector>

using namespace ns3;

class P2PoolNode : public Application
{
  public:
    static TypeId GetTypeId(void);

    P2PoolNode(uint32_t P2PoolNodeId,
               Ptr<NormalRandomVariable> shareGenTimeModel,
               Ptr<RandomVariableStream> latencyModel,
               uint32_t maxTipsToReference,
               time_t max_share_time);
    virtual ~P2PoolNode();

    // Set up TCP connections to peers
    void SetupConnections(const std::vector<Ptr<P2PoolNode>>& peers,
                          std::map<uint32_t, std::vector<Ipv4Address>>& nodeToAddresses,
                          uint16_t port);

    // Get P2PoolNode ID
    uint32_t GetNodeId() const;

    // Get local ShareChain
    ShareChain* GetShareChain() const;

    // Start generating shares
    void StartShareGeneration(double interval);

    // Stop generating shares
    void StopShareGeneration();

    // Get orphan count
    uint32_t getOrphanCount() const;

    // Print chain stats
    void PrintChainStats() const;

  protected:
    virtual void StartApplication(void);
    virtual void StopApplication(void);

  private:
    // Generate a new share and broadcast to all peers
    void GenerateAndBroadcastShare();

    // Schedule next share generation
    void ScheduleNextShareGeneration();

    // Broadcast a share to all peers
    void BroadcastShare(Share* share);

    // Send a share to a specific peer
    void SendShareToPeer(Share* share, Address peerAddress);

    // Handle received share from peer
    void HandleReceivedShare(Ptr<Socket> socket);

    // New connection callback
    void ConnectionAcceptedCallback(Ptr<Socket> socket, const Address& address);

    bool ConnectionRequestCallback(Ptr<Socket> socket, const Address& address);

    // Serialize share for network transmission
    std::string SerializeShare(Share* share);

    void ConnectionSucceeded(Ptr<Socket> socket);
    void ConnectionFailed(Ptr<Socket> socket);

    // Deserialize share from network data
    Share* DeserializeShare(const std::string& data);

    uint32_t GenerateUniqueShareId();

    // P2PoolNode ID
    uint32_t m_nodeId;

    void DoSetupConnections(const std::vector<Ptr<P2PoolNode>>& peers,
                            std::map<uint32_t, std::vector<Ipv4Address>>& nodeToAddresses,
                            uint16_t port);

    // Local sharechain
    ShareChain* m_shareChain;

    // Maximum tips to reference when creating a share
    uint32_t m_maxTipsToReference;

    // Random variable for share generation times
    Ptr<NormalRandomVariable> m_shareGenTimeModel;

    // Latency model for this P2PoolNode's communication
    Ptr<RandomVariableStream> m_latencyModel;

    // TCP listening socket
    Ptr<Socket> m_socket;

    // Map of connected peers (socket -> address)
    std::map<Address, Ptr<Socket>> m_peerSockets;

    // Event ID for next share generation
    EventId m_nextShareEvent;

    // Running status
    bool m_running;

    // Latest share ID
    uint64_t m_nextShareId;

    // maximum time_stamp a share can i have for this simulation
    time_t maxTime;

    // Statistics
    uint32_t m_sharesCreated;
    uint32_t m_sharesReceived;
    uint32_t m_sharesSent;
};

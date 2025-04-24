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
               uint32_t maxTipsToReference,
               ns3::Time max_share_time);
    virtual ~P2PoolNode();

    // Get P2PoolNode ID
    uint32_t GetNodeId() const;

    // Get local ShareChain
    ShareChain* GetShareChain() const;

    void AddPeerSocket(uint32_t peerId, Ptr<Socket> socket);

    // Stop generating shares
    void StopShareGeneration();

    // Get orphan count
    uint32_t getOrphanCount() const;

    // Print chain stats
    void PrintChainStats() const;

    // Handle received share from peer
    void HandleReceivedShare(Ptr<Socket> socket);

    // Schedule next share generation
    void ScheduleNextShareGeneration();

  protected:
    virtual void StartApplication(void);
    virtual void StopApplication(void);

  private:
    // Generate a new share and broadcast to all peers
    void GenerateAndBroadcastShare();

    // Broadcast a share to all peers
    void BroadcastShare(Share* share);

    // Send a share to a specific peer
    void SendShareToPeer(Share* share, Ptr<Socket> socket);

    // New connection callback
    void ConnectionAcceptedCallback(Ptr<Socket> socket, const Address& address);

    bool ConnectionRequestCallback(Ptr<Socket> socket, const Address& address);

    // Serialize share for network transmission
    std::string SerializeShare(Share* share);

    void ConnectionSucceeded(Ptr<Socket> socket);

    void ConnectionFailed(Ptr<Socket> socket);

    // Deserialize share from network data
    Share* DeserializeShare(const std::string& data);

    //Generates unique shareid when creating new shareId
    uint32_t GenerateUniqueShareId();

    // P2PoolNode ID
    uint32_t nodeId;

    // Local sharechain
    ShareChain* shareChain;

    // Maximum tips to reference when creating a share
    uint32_t maxTipsToReference;

    // Random variable for share generation times
    Ptr<NormalRandomVariable> shareGenTimeModel;

    // TCP listening socket
    Ptr<Socket> socket;

    // Map of connected peers (nodeID -> socket)
    std::unordered_map<uint32_t, Ptr<Socket>> peerSockets;

    // Event ID for next share generation
    EventId nextShareEvent;

    std::unordered_set<uint32_t> existingShares;

    // Running status
    bool running;
    // maximum time_stamp a share can i have for this simulation
    ns3::Time maxTime;

    // Statistics
    uint32_t sharesCreated;
    uint32_t sharesReceived;
    uint32_t sharesSent;
};

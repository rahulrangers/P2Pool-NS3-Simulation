#include "node.h"

#include "ns3/applications-module.h"
#include "ns3/config-store-module.h"
#include "ns3/core-module.h"
#include "ns3/internet-module.h"
#include "ns3/log.h"
#include "ns3/mobility-module.h"
#include "ns3/network-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/random-variable-stream.h"

#include <iostream>
#include <utility>
#include <map>
#include <memory>
#include <string>
#include <vector>

using namespace ns3;

class P2PManager
{
  public:
    /**
     * Constructor to initialize the P2PManager with simulation parameters.
     *
     * @param numNodes Number of nodes in the network.
     * @param shareGenMean Mean interval for share generation.
     * @param shareGenVariance Variance in share generation interval.
     * @param maxTipsToReference Maximum number of tips a new share can reference.
     * @param simulationDuration Duration of the simulation in seconds.
     * @param maxTimeStamp share with maxtimestamp which can be added to sharechain
     */
    P2PManager(uint32_t numNodes,
               double shareGenMean,
               double shareGenVariance,
               uint32_t maxTipsToReference,
               uint32_t simulationDuration,
               Time maxTimeStamp);

    /**
     * Sets up the simulation environment including node creation,
     * network setup, and initial configuration.
     */
    void CreateRandomTopology(double connectionProbability = 0.3, double latency = 5.0);

    /**
     * Runs the simulation for the specified duration.
     */
    void Run();
    
    /**
     * Prints out the results/statistics of the simulation after execution.
     */
    void PrintResults();

  private:
    
    uint32_t numNodes;
    double shareGenMean;
    double shareGenVariance;
    uint32_t maxTipsToReference;
    uint32_t simulationDuration;
    NodeContainer nodes;
    std::vector<Ptr<P2PoolNode>> p2pNodes;
    std::vector<Ipv4Address> nodeAddresses;
    Ipv4AddressHelper addressHelper;
    InternetStackHelper internet;
    Time maxTime;
    struct ConnectionInfo
    {
        NetDeviceContainer devices;
        Ptr<PointToPointChannel> channel;
        Ipv4InterfaceContainer ifc;
    };

    std::map<std::pair<uint32_t, uint32_t>, ConnectionInfo> connections;
     
    /**
      * Each peer starts generating shares
     */
    void startGeneratingShares();

     /**
     * Creates a TCP connection for each link in the connections.
     */
    void makeconnections();
    
    /**
    * Creates a links between i and j using pointtopoint Helper
    */
    void ConnectNodes(uint32_t i, uint32_t j, double latencyMs);
    
    /**
    * Creates a TCP connection for i and j.
    */
    void ConnectPeerSockets(uint32_t i, uint32_t j);

    /**
     * Creates a normal distribution model for share generation interval.
     *
     * @param nodeId ID of the node.
     * @return A pointer to a NormalRandomVariable for share timing.
     */
    Ptr<NormalRandomVariable> CreateShareGenTimeModel(uint32_t nodeId);
};

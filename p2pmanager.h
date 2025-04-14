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
     * @param meanLatency Mean network latency between nodes.
     * @param latencyVariance Variance in network latency.
     * @param shareGenMean Mean interval for share generation.
     * @param shareGenVariance Variance in share generation interval.
     * @param maxTipsToReference Maximum number of tips a new share can reference.
     * @param simulationDuration Duration of the simulation in seconds.
     */
    P2PManager(uint32_t numNodes,
               double meanLatency,
               double latencyVariance,
               double shareGenMean,
               double shareGenVariance,
               uint32_t maxTipsToReference,
               uint32_t simulationDuration);

    /**
     * Sets up the simulation environment including node creation,
     * network setup, and initial configuration.
     */
    void Configure();

    /**
     * Runs the simulation for the specified duration.
     */
    void Run();

    /**
     * Establishes point-to-point connections between all nodes,
     * and records the IP addresses mapped to each node.
     *
     * @param nodeToAddresses Mapping of node IDs to their assigned IP addresses.
     */
    void SetupAllConnections(std::map<uint32_t, std::vector<Ipv4Address>>& nodeToAddresses);

    /**
     * Prints out the results/statistics of the simulation after execution.
     */
    void PrintResults();

  private:
    
    uint32_t m_numNodes;
    double m_meanLatency;
    double m_latencyVariance;
    double m_shareGenMean;
    double m_shareGenVariance;
    uint32_t m_maxTipsToReference;
    uint32_t m_simulationDuration;

    NodeContainer m_nodes;
    std::vector<Ptr<P2PoolNode>> m_p2pNodes;
    std::vector<Ipv4Address> m_nodeAddresses;
    NetDeviceContainer m_devices;
    InternetStackHelper m_internet;

    /**
     * Creates a random variable model for latency for a specific node.
     *
     * @param nodeId ID of the node.
     * @return A pointer to a RandomVariableStream modeling latency.
     */
    Ptr<RandomVariableStream> CreateLatencyModel(uint32_t nodeId);

    /**
     * Creates a normal distribution model for share generation interval.
     *
     * @param nodeId ID of the node.
     * @return A pointer to a NormalRandomVariable for share timing.
     */
    Ptr<NormalRandomVariable> CreateShareGenTimeModel(uint32_t nodeId);
};

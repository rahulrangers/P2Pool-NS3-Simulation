#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/internet-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/applications-module.h"
#include "ns3/config-store-module.h"

#include <iostream>
#include <string>
#include "p2pmanager.h"  

using namespace ns3;

int main(int argc, char *argv[]) {
  uint32_t numNodes = 50;
  double meanLatency = 0.5;        
  double latencyVariance = 0.1;    
  double shareGenMean = 1.0;       
  double shareGenVariance = 0.5;    
  uint32_t maxTipsToReference = 100; 
  uint32_t simDuration = 100000000;       


  LogComponentEnable("P2PManager", LOG_LEVEL_INFO);
  LogComponentEnable("Node", LOG_LEVEL_INFO);


  std::cout << "=== P2Pool Simulation Parameters ===" << std::endl;
  std::cout << "Number of nodes: " << numNodes << std::endl;
  std::cout << "Mean latency: " << meanLatency << " seconds" << std::endl;
  std::cout << "Latency variance: " << latencyVariance << std::endl;
  std::cout << "Mean share generation time: " << shareGenMean << " seconds" << std::endl;
  std::cout << "Share generation variance: " << shareGenVariance << std::endl;
  std::cout << "Max tips to reference: " << maxTipsToReference << std::endl;
  std::cout << "Simulation duration: " << simDuration << " seconds" << std::endl;
  std::cout << "===================================" << std::endl;


  uint32_t adjustedDuration = simDuration + std::ceil(meanLatency * 5);
  std::cout << "Adjusted simulation duration: " << adjustedDuration << " seconds" << std::endl;


  P2PManager p2pManager(numNodes, meanLatency, latencyVariance, 
                        shareGenMean, shareGenVariance, 
                        maxTipsToReference, adjustedDuration);
  
  p2pManager.Configure();
  

  std::cout << "Starting simulation..." << std::endl;
  p2pManager.Run();
  
  p2pManager.PrintResults();
  
  return 0;
}

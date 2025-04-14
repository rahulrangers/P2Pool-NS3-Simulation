# P2Pool Simulation Project

This project is a simulation of a P2P (peer-to-peer) mining pool network using the NS-3 (Network Simulator 3) framework. It models the behavior of a distributed share chain system similar to P2Pool, allowing for testing and analysis of different network configurations and parameters.

## Overview

The simulation implements a network of mining nodes that generate shares, reference previous shares, and maintain a decentralized share chain. Each node independently mines shares and broadcasts them to other nodes in the network, with configurable parameters for network latency, share generation rate, and other factors that affect real-world mining pool performance.

## Components

### Core Classes

1. **Share** (`share.h`)
   - Represents a share in the mining pool
   - Contains share ID, sender ID, timestamp, and references to previous shares

2. **ShareChain** (`sharechain.h`)
   - Manages the share chain data structure
   - Validates and adds new shares
   - Tracks chain tips and orphaned shares
   - Uses Boost Graph Library for chain representation

3. **P2PoolNode** (`node.h`)
   - Implements a mining node in the network
   - Generates shares and broadcasts them to peers
   - Processes received shares
   - Maintains connections with other nodes

4. **P2PManager** (`p2pmanager.h`)
   - Orchestrates the entire simulation
   - Configures the network topology
   - Sets up connections between nodes
   - Collects and presents simulation results

### Network Simulation

The project uses NS-3 for network simulation, including:
- TCP socket communication between nodes
- Configurable network latency and variance
- Point-to-point connections between nodes
- IP addressing and routing

## How the ShareChain Works

### Share Generation and Propagation

1. **Share Generation Model**
   - Each node has a `shareGenTimeModel` that follows a normal distribution
   - Parameters `shareGenMean` and `shareGenVariance` control the distribution
   - Node's "mining power" affects its share generation rate
   - Generated shares reference the current chain tips

2. **Network Latency Model**
   - Each node has a `latencyModel` that follows a normal distribution
   - Parameters `meanLatency` and `latencyVariance` control the distribution
   - Latency creates realistic delays in share propagation across the network
   - Different nodes receive the same share at different times

### ShareChain Construction

1. **Genesis Share Creation**
   - Each node's ShareChain begins with a common genesis share (ID 0)
   - All subsequent shares must trace back to this genesis share

2. **Share Validation and Addition**
   - When a node receives a share, it validates all previous references
   - If all referenced shares are already in the chain, the new share is added
   - If any referenced share is missing, the new share is placed in a pending queue
   - Pending shares are processed once all their prerequisites are met

3. **Chain Tips Management**
   - The ShareChain tracks all chain tips (shares not referenced by any other share)
   - When a new share is added, it references existing tips, removing them from the tips list
   - The new share then becomes a tip itself

4. **Subtree Weight Calculation**
   - For each share, a BFS traversal is performed from the share to the genesis
   - The number of shares encountered during traversal is the "weight" of the subtree
   - This weight determines the main chain and is used for resolving conflicts

### Orphan Share Determination

1. **Orphan Calculation**
   - Orphaned shares are valid shares that are not part of the main chain
   - Formula: `orphan_count = total_shares - weight_of_heaviest_tip`
   - Each node calculates its own orphan count based on its local share chain

2. **Timestamp Consistency**
   - To ensure consistent graphs across all nodes, a maximum share timestamp is enforced
   - Shares with timestamps beyond this limit are rejected

## Running the Simulation

To run the simulation:

```bash
./ns3 --run "scratch/p2pool-sim/main.cc"
```

### Configuration Parameters

You can modify the following parameters in `main.cpp`:

- `numNodes`: Number of mining nodes in the network
- `meanLatency`: Average network latency between nodes (seconds)
- `latencyVariance`: Variance in network latency
- `shareGenMean`: Average time to generate a share (seconds)
- `shareGenVariance`: Variance in share generation time
- `maxTipsToReference`: Maximum number of tips each share can reference
- `simDuration`: Duration of the simulation (seconds)

## Simulation Output

The simulation produces output including:
- Per-node statistics (shares created, received, sent)
- Orphan share counts for each node
- Average orphan rate across the network
- Log information about share propagation and network events


## Project Structure

```
project/
├── share.h          # Share class definition
├── share.cc         # Share implementation
├── sharechain.h     # ShareChain class definition
├── sharechain.cc    # ShareChain implementation
├── node.h           # P2PoolNode class definition
├── node.cc          # P2PoolNode implementation
├── p2pmanager.h     # P2PManager class definition
├── p2pmanager.cc    # P2PManager implementation
├── main.cc          # Main simulation entry point
└── README.md        # This file
```
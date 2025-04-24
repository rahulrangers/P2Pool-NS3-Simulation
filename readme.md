# P2Pool Simulation Project

This project is a simulation of a P2P (peer-to-peer) mining pool network using the NS-3 (Network Simulator 3) framework. It models the behavior of a distributed share chain system similar to P2Pool, allowing for testing and analysis of different network configurations and parameters.

## Overview

The simulation implements a network of mining nodes that generate shares, reference previous shares, and maintain a decentralized share chain. Each node independently mines shares and broadcasts them to other nodes in the network, with configurable parameters for network latency, share generation rate, and other factors that affect real-world mining pool performance. The system uses a gossip protocol for share propagation through a randomized network topology, rather than a full mesh network.

## Components

### Core Classes

1. **Share** (`share.h`)
   - Represents a share in the mining pool
   - Contains share ID, sender ID, timestamp, parent ID, and references to previous shares

2. **ShareChain** (`sharechain.h`)
   - Manages the share chain data structure
   - Validates and adds new shares
   - Tracks chain tips and orphaned shares
   - Uses Boost Graph Library for chain representation
   - Calculates main chain length and uncle blocks

3. **P2PoolNode** (`node.h`)
   - Implements a mining node in the network
   - Generates shares and broadcasts them to peers using gossip protocol
   - Processes received shares
   - Maintains connections with other nodes

4. **P2PManager** (`p2pmanager.h`)
   - Orchestrates the entire simulation
   - Configures the random network topology
   - Sets up connections between nodes
   - Collects and presents simulation results

### Network Simulation

The project uses NS-3 for network simulation, including:
- TCP socket communication between nodes
- Configurable network latency
- Point-to-point connections between nodes with random topology
- IP addressing and routing
- Gossip protocol implementation for share propagation

## How the ShareChain Works

### Share Generation and Propagation

1. **Share Generation Model**
   - Each node has a `shareGenTimeModel` that follows a normal distribution
   - Parameters `shareGenMean` and `shareGenVariance` control the distribution
   - Node's "mining power" affects its share generation rate
   - Generated shares reference the current chain tips and specify a parent ID

2. **Gossip Protocol**
   - Instead of a full mesh network, nodes connect to a subset of other nodes
   - Connection probability controls network density
   - When a node generates or receives a new share, it forwards to all its connected peers
   - This creates an efficient epidemic-style propagation through the network

3. **Network Latency Model**
   - Network connections have configurable latency
   - Latency creates realistic delays in share propagation across the network
   - Different nodes receive the same share at different times

### ShareChain Construction

1. **Genesis Share Creation**
   - Each node's ShareChain begins with a common genesis share (ID 1)
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
   - Each tip has a weight based on its subtree size

4. **Subtree Weight Calculation**
   - For each share, a BFS traversal is performed to calculate subtree size
   - The number of shares encountered during traversal is the "weight" of the subtree
   - This weight determines the main chain and is used for resolving conflicts

### Orphan Share Determination

1. **Orphan Calculation**
   - Orphaned shares are valid shares that are not part of the main chain
   - Calculated as: `orphan_count = total_shares - main_chain_length - uncle_blocks`
   - Each node calculates its own orphan count based on its local share chain

2. **Uncle Blocks**
   - Shares that are referenced but not in the main chain are counted as uncle blocks
   - These contribute to the overall security of the chain

3. **Timestamp Consistency**
   - To ensure consistent graphs across all nodes, a maximum share timestamp is enforced
   - Shares with timestamps beyond this limit are rejected

## Running the Simulation

To run the simulation:

```bash
./ns3 --run "scratch/p2pool/main.cc"
```

### Configuration Parameters

You can modify the following parameters in `main.cc`:

- `numNodes`: Number of mining nodes in the network (default: 50)
- `latency`: Network latency between nodes (milliseconds) (default: 50)
- `shareGenMean`: Average time to generate a share (seconds) (default: 1)
- `shareGenVariance`: Variance in share generation time (default: 5)
- `maxTipsToReference`: Maximum number of tips each share can reference (default: 10000)
- `simDuration`: Duration of the simulation (seconds) (default: 500)
- `maxTimeStamp`: Maximum timestamp for valid shares (default: simDuration/10)

## Simulation Output

The simulation produces output including:
- Per-node statistics (shares created, received, sent)
- Orphan share counts for each node
- Uncle block counts
- Main chain length
- Total shares in the system
- Average orphan rate across the network
- shares of the main chain

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
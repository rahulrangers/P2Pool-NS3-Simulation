#ifndef SHARECHAIN_H
#define SHARECHAIN_H

#include <boost/graph/adjacency_list.hpp>
#include <boost/graph/graph_traits.hpp>
#include <boost/graph/topological_sort.hpp>
#include <vector>
#include <unordered_map>
#include <unordered_set>
#include <memory>
#include <utility>
#include "share.h"

/**
 * Class to represent the ShareChain in the P2Pool network
 * Uses Boost Graph Library to maintain the DAG of shares
 */
class ShareChain {
public:
    struct VertexProperties {
        Share* share;
    };

    using ShareGraph = boost::adjacency_list<
        boost::vecS,           
        boost::vecS,           
        boost::bidirectionalS, 
        VertexProperties       
    >;

    // Define vertex and edge descriptors for easier access
    using Vertex = boost::graph_traits<ShareGraph>::vertex_descriptor;
    using Edge = boost::graph_traits<ShareGraph>::edge_descriptor;

    /**
     * Constructor to initialize a ShareChain with a genesis node
     */
    ShareChain(time_t max_time);
    /**
     * Adds a share to the chain
     * @param share Pointer to the share to be added
     * @return true if share was successfully added, false otherwise
     */
    bool addShare(Share* share);
    
    /**
     * Gets the current main chain tip(s)
     * @return Vector of pairs containing weight and share pointer of main chain tips
     */
    const std::unordered_map<uint32_t,uint32_t> getChainTips() const;
    
    /**
     * Gets the count of orphaned shares (shares not in the main chain)
     * @return Number of orphaned shares
     */
    size_t getOrphanCount() ;
    
    /**
     * Gets the total number of shares in the chain
     * @return Total number of shares
     */
    size_t getTotalShares() const;
    /**
     * Gets all shares in the chain
     * @return Map of all shares indexed by their IDs
     */
    const std::unordered_map<uint32_t, Vertex>& getAllShareVertices() const;
    
    /**
     * Gets the genesis share
     * @return Pointer to the genesis share
     */
    Share* getGenesisShare() const;

     /**
     * Gets maxtimestamp
     * @return return maxtimestamp
     */
    void setmaxtimestamp(time_t maxtime) ;

private:
    // The graph that stores our DAG of shares
    ShareGraph graph_;
    
    // Maps share IDs to their vertices in the graph
    std::unordered_map<uint32_t, Vertex> shareToVertex_;
    
    // Current tipsID of the sharechain with their weights 
    std::unordered_map<uint32_t,uint32_t>  ChainTips;
    
    // Pending shares that couldn't be added due to missing previous references
    std::unordered_map<uint32_t, Share*> pendingShares;
    
    // Genesis share
    Share* genesisShare_;
    
    // Total number of shares in the chain
    size_t totalShares_;
    
    //share's maxiumum timestamp limit
    time_t max_share_timestamp;
    
    /**
     * Calculates the weight of a subtree (number of nodes from this share to genesis)
     * @param v Starting vertex
     * @return Weight of the subtree
     */
    uint32_t calculateSubtreeWeight(Vertex v);
    
    /**
     * Updates the main chain based on a newly added share
     * @param share Newly added share
     * @param vertex Vertex of the newly added share
     */
    void updateChainTips(Share* share, Vertex vertex);
    /**
     * Validates that all previous shares referenced by a share exist in the graph
     * @param share Share to validate
     * @return true if all prior references exist, false otherwise
     */
    bool validatePrevRefs(const Share* share) const;
    
    /**
     * Process any pending shares that might now be valid after adding a new share
     */
    void processPendingShares();
    
    /**
     *Traverse through mainchain and finds uncleBlocks
     */
    uint32_t getUncleBlocks() ;

    /**
     * Creates and adds the genesis share to the chain
     */
    void createGenesisShare();

    /**
     * Gets tip with heaviest Subtree
     */
    uint32_t getBestTip();

     /**
     * Gets mainchain length
     */
    uint32_t MainChainLength();
};

#endif 
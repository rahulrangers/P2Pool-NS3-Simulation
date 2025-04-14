#include <iostream>
#include "sharechain.h"
#include <boost/graph/breadth_first_search.hpp>
#include <boost/graph/dijkstra_shortest_paths.hpp>
#include <algorithm>
#include <queue>
#include <unordered_set>
#include <limits>

ShareChain::ShareChain(time_t max_time) 
    : totalShares_(0), genesisShare_(nullptr), max_share_timestamp(max_time) {
    createGenesisShare();
}

void ShareChain::createGenesisShare() {
    genesisShare_ = new Share(0, 0, 0, std::vector<uint32_t>());
    Vertex genesisVertex = boost::add_vertex({genesisShare_}, graph_);
    shareToVertex_[0] = genesisVertex;
    ChainTips_[genesisShare_->getShareId()]=1;
    totalShares_ = 1;
}

bool ShareChain::addShare(Share* share) {
    if (!share) return false;
    if(max_share_timestamp < share->getTimestamp() ) return false;
    uint32_t shareId = share->getShareId();
    if (shareToVertex_.find(shareId) != shareToVertex_.end()) return false; 
    
    if (!validatePrevRefs(share)) {
        pendingShares_[shareId] = share;
        return false;
    }
    
    Vertex newVertex = boost::add_vertex({share}, graph_);
    shareToVertex_[shareId] = newVertex;
    totalShares_++;
    
    for (uint32_t prevId : share->getPrevRefs()) {
            if (shareToVertex_.find(prevId) != shareToVertex_.end()) {
                Vertex prevVertex = shareToVertex_[prevId];
                boost::add_edge(newVertex, prevVertex, graph_);
            }
    }
    
    updateMainChain(share, newVertex);
    processPendingShares();
    
    return true;
}

const std::unordered_map<uint32_t,uint32_t> ShareChain::getChainTips() const {
    return ChainTips_;
}

size_t ShareChain::getOrphanCount() const {
    uint32_t maxi=0;
    for(std::pair<uint32_t,uint32_t> i : ChainTips_) maxi=std::max(maxi,i.second);
    return  totalShares_- maxi;
}

size_t ShareChain::getTotalShares() const {
    return totalShares_;
}
void ShareChain::setmaxtimestamp(time_t maxtime) {
     max_share_timestamp = maxtime;
}

const std::unordered_map<uint32_t, ShareChain::Vertex>& ShareChain::getAllShareVertices() const {
    return shareToVertex_;
}

Share* ShareChain::getGenesisShare() const {
    return genesisShare_;
}

uint32_t ShareChain::calculateSubtreeWeight(Vertex v) {
    if (v == boost::graph_traits<ShareGraph>::null_vertex()) {
        return 0;
    }
    
    std::unordered_set<Vertex> visited;
    std::queue<Vertex> queue;
    
    queue.push(v);
    visited.insert(v);
    
    while (!queue.empty()) {
        Vertex current = queue.front();
        queue.pop();
        
        boost::graph_traits<ShareGraph>::out_edge_iterator ei, ei_end;
        for (boost::tie(ei, ei_end) = boost::out_edges(current, graph_); ei != ei_end; ++ei) {
            Vertex target = boost::target(*ei, graph_);
            if (visited.find(target) == visited.end()) {
                visited.insert(target);
                queue.push(target);
            }
        }
    }
    return visited.size();
}

void ShareChain::updateMainChain(Share* share, Vertex vertex) {
    int weight = calculateSubtreeWeight(vertex);
    for (uint32_t prevId : share->getPrevRefs()) {
        if(ChainTips_.find(prevId)!=ChainTips_.end())
        ChainTips_.erase(prevId);
    }
    ChainTips_[share->getShareId()] = weight;
}

bool ShareChain::validatePrevRefs(const Share* share) const {
    if (!share) return false;

    for (uint32_t prevId : share->getPrevRefs()) {
            if (shareToVertex_.find(prevId) == shareToVertex_.end()) {
                return false; 
            }
    }
    
    return true;
}

void ShareChain::processPendingShares() {
    bool progress = true;
    std::vector<uint32_t> processed;
    while (progress) {
        progress = false;
        processed.clear();
        for (auto& pair : pendingShares_) {
            Share* pendingShare = pair.second;
            if (addShare(pendingShare)) {
                processed.push_back(pair.first);
                progress = true;
            }
        }
        for (uint32_t id : processed) {
            pendingShares_.erase(id);
        }
    }
}

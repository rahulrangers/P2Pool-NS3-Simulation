#include <iostream>
#include "sharechain.h"
#include <algorithm>
#include <queue>
#include "ns3/simulator.h"
#include "ns3/log.h"
#include <boost/graph/graphviz.hpp>
#include <unordered_set>
#include <limits>
#include <filesystem> 
namespace fs = std::filesystem;

NS_LOG_COMPONENT_DEFINE("ShareChain");

ShareChain::ShareChain(ns3::Time max_time) 
    : totalShares(0), genesisShare(nullptr), max_share_timestamp(max_time) {
    createGenesisShare();
}

void ShareChain::createGenesisShare() {
    genesisShare = new Share(1, 0, ns3::Seconds(0), std::vector<uint32_t>(), 0);
    Vertex genesisVertex = boost::add_vertex({genesisShare}, graph);
    shareToVertex[1] = genesisVertex;
    ChainTips[genesisShare->getShareId()]=1;
    totalShares = 1;
}

bool ShareChain::addShare(Share* share) {
    if (!share) return false;
    if(max_share_timestamp < share->getTimestamp() ) {
        return false;
    }
    uint32_t shareId = share->getShareId();
    if (shareToVertex.find(shareId) != shareToVertex.end()) return false; 
    
    if (!validatePrevRefs(share)) {
        pendingShares[shareId] = share;
        return false;
    }
    
    totalShares++;
    Vertex newVertex = boost::add_vertex({share}, graph);
    shareToVertex[shareId] = newVertex;
    for (uint32_t prevId : share->getPrevRefs()) {
            if (shareToVertex.find(prevId) != shareToVertex.end()) {
                Vertex prevVertex = shareToVertex[prevId];
                boost::add_edge(newVertex, prevVertex, graph);
            }

    }

    updateChainTips(share, newVertex);
    processPendingShares();
    
    return true;
}

const std::unordered_map<uint32_t,uint32_t> ShareChain::getChainTips() const {
    return ChainTips;
}

size_t ShareChain::getOrphanCount() {
    uint32_t uncleBlocks = getUncleBlocks(); 
    uint32_t mainchainblocks = MainChainLength();
    return  totalShares - uncleBlocks - mainchainblocks; 
}

size_t ShareChain::getTotalShares() const {
    return totalShares;
}

void ShareChain::setmaxtimestamp(ns3::Time maxtime) {
     max_share_timestamp = maxtime;
}

const std::unordered_map<uint32_t, ShareChain::Vertex>& ShareChain::getAllShareVertices() const {
    return shareToVertex;
}

Share* ShareChain::getGenesisShare() const {
    return genesisShare;
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
        for (boost::tie(ei, ei_end) = boost::out_edges(current, graph); ei != ei_end; ++ei) {
            Vertex target = boost::target(*ei, graph);
            if (visited.find(target) == visited.end()) {
                visited.insert(target);
                queue.push(target);
            }
        }
    }
    return visited.size();
}

void ShareChain::updateChainTips(Share* share, Vertex vertex) {
    int weight = calculateSubtreeWeight(vertex);
    for (uint32_t prevId : share->getPrevRefs()) {
        if(ChainTips.find(prevId)!=ChainTips.end())
        ChainTips.erase(prevId);
    }
    ChainTips[share->getShareId()] = weight;
}

bool ShareChain::validatePrevRefs(const Share* share) const {
    if (!share) return false;

    for (uint32_t prevId : share->getPrevRefs()) {
            if (shareToVertex.find(prevId) == shareToVertex.end()) {
                return false; 
            }
    }
    
    return true;
}

uint32_t ShareChain::getBestTip() {
    uint32_t max_weight=0;
    uint32_t chosen_tip=0;
    for(std::pair<uint32_t,uint32_t> i : ChainTips) {
        if(max_weight < i.second) {
            max_weight=i.second;
            chosen_tip = i.first;
        }
    }
    return chosen_tip;
}

uint32_t ShareChain::MainChainLength() {
    uint32_t chosen_tip = getBestTip();
    uint32_t chain_length=1;
    Share* mainShare = graph[shareToVertex[chosen_tip]].share;
    while(mainShare->getShareId()!=1){
    uint32_t parentid = mainShare->getParentId();
    chain_length +=1;
    mainShare =  graph[shareToVertex[parentid]].share;
    }
    return chain_length;
}

std::vector<uint32_t> ShareChain::showchain() {
    std::vector<uint32_t> ans;
    uint32_t chosen_tip = getBestTip();
    uint32_t chain_length=1;
    Share* mainShare = graph[shareToVertex[chosen_tip]].share;
    while(mainShare->getShareId()!=1){
    ans.push_back(mainShare->getShareId());
    uint32_t parentid = mainShare->getParentId();
    chain_length +=1;
    mainShare =  graph[shareToVertex[parentid]].share;
    }
    ans.push_back(1);
    return ans;
}

uint32_t ShareChain::getUncleBlocks() {
    uint32_t chosen_tip = getBestTip();
    uint32_t UncleBlocks=0;
    Share* mainShare = graph[shareToVertex[chosen_tip]].share;
    while(mainShare->getShareId()!=1){
    uint32_t parentid = mainShare->getParentId();
    UncleBlocks +=(mainShare->getPrevRefs().size()-1);
    mainShare =  graph[shareToVertex[parentid]].share;
    }
    return UncleBlocks;
}

void ShareChain::processPendingShares() {
    bool progress = true;
    std::vector<uint32_t> processed;
    while (progress) {
        progress = false;
        processed.clear();
        for (auto& pair : pendingShares) {
            Share* pendingShare = pair.second;
            if (addShare(pendingShare)) {
                processed.push_back(pair.first);
                progress = true;
            }
        }
        for (uint32_t id : processed) {
            pendingShares.erase(id);
        }
    }
}


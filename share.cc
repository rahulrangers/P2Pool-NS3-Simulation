#include "share.h"
#include <vector>
#include <ctime>
#include <cstdint>
#include "ns3/simulator.h"

Share::Share(uint64_t shareId, uint32_t senderId, ns3::Time timestamp, 
    const std::vector<uint32_t>& prevShares, uint32_t parId)
: shareId(shareId), senderId(senderId), timestamp(timestamp), prevShares(prevShares) , parentId(parId) {}

uint32_t Share::getSenderId() const {
    return senderId;
}

uint32_t Share::getShareId() const {
    return shareId;
}

const std::vector<uint32_t>& Share::getPrevRefs() const {
    return prevShares;
}
ns3::Time Share::getTimestamp() const{
    return timestamp;
}

void Share::addPrevRef(uint32_t shareid) {
        prevShares.push_back(shareid);
}

uint32_t Share::getParentId() const {
  return parentId;
}
#include "share.h"
#include <vector>
#include <ctime>
#include <cstdint>

Share::Share(uint64_t shareId, uint32_t senderId, time_t timestamp, 
    const std::vector<uint32_t>& prevShares)
: shareId_(shareId), senderId_(senderId), timestamp_(timestamp), prevShares_(prevShares) {}

uint32_t Share::getSenderId() const {
    return senderId_;
}

uint32_t Share::getShareId() const {
    return shareId_;
}

const std::vector<uint32_t>& Share::getPrevRefs() const {
    return prevShares_;
}
time_t Share::getTimestamp() const{
    return timestamp_;
}

void Share::addPrevRef(uint32_t shareid) {
        prevShares_.push_back(shareid);
}
#ifndef SHARE_H
#define SHARE_H

#include <vector>
#include <ctime>
#include <cstdint>
#include "ns3/simulator.h"

/**
 * Class to represent a Share in the P2Pool network
 */
class Share {
public:
    /**
     * Constructor to initialize a Share.
     * @param senderId 
     * @param timestamp 
     * @param prevShares 
     */
    Share(uint64_t shareId, uint32_t senderId, ns3::Time timestamp, 
          const std::vector<uint32_t>& prevShares,uint32_t parentId);

    /**
     * Returns the unique ID of the share.
     */
    uint32_t getShareId() const;

    /**
     * Returns the ID of the sender (node who created this share).
     */
    uint32_t getSenderId() const;

    /**
     * Returns the timestamp of when this share was created.
     */
    ns3::Time getTimestamp() const;

    /**
     * Returns the vector of previous shares (i.e., references to other shares).
     */
    const std::vector<uint32_t>& getPrevRefs() const;

    /**
     * Add a reference to previous shares. (Placeholder: implementation not shown here)
     */
    void addPrevRef(uint32_t shareid) ;

    /**
     * Less-than comparison operator based on timestamp.
     */
    bool operator<(const Share& other) const {
        return timestamp < other.timestamp;
    }

    /**
     * Get ParentId which this share connects as part of mainchain.
     */
    uint32_t getParentId() const;
    
private:
    uint32_t shareId;                
    uint32_t senderId;               
    ns3::Time timestamp;                
    std::vector<uint32_t> prevShares;   
    uint32_t parentId;
};

#endif 

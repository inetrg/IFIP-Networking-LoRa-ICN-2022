#ifndef CCNSIM_DSME_CORE_LAYER_H
#define CCNSIM_DSME_CORE_LAYER_H
#include <omnetpp.h>
#include "core_layer.h"
#include "ccnsim_dsme.h"

/**
     * Derive the ccnsim_to_inet class from cSimpleModule.
     */
class ccnsim_dsme_core_layer : public core_layer
{
protected:
    // The following redefined virtual function holds the algorithm.
    virtual void initialize() override;
    virtual void handleMessage(cMessage *msg) override;
    // PK indication and push timer
    cMessage *timer;
    std::vector<int> my_contents;
    int count=0;
    bool it_has_a_repo_attached{false};
    void send_interest_indication(chunk_t chunk, int ogate_idx);
    void send_push(chunk_t chunk, int ogate_idx);
    void send_indication_or_push(chunk_t chunk, int type, int ogate_idx);
    int i_am_src_repo(name_t name);
    virtual void handle_data(ccn_data *data_msg) override;
    virtual void handle_interest(ccn_interest *int_msg) override;
    virtual void finish() override;
    simtime_t timeToNext;
    simtime_t warmUpTime;
};
#endif

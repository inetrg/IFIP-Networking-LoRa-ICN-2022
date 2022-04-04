#ifndef LORADSMERADIO_H
#define LORADSMERADIO_H

#include "LoRa/LoRaRadio.h"

namespace inet {

namespace physicallayer {

class INET_API LoRaDSMERadio : public LoRaRadio {
public:
    void aggregateToA(simtime_t T);
protected:
    virtual void initialize(int stage) override;
    virtual void handleSelfMessage(cMessage *message) override;
private:
    cMessage *dutyCycleTimer;
    simtime_t aggregateTime{0};
    simsignal_t dutyCycle;
};
}
}

#endif

#include "LoRaDSMERadio.h"

#define DUTY_CYCLE_PERIOD (3600U)

namespace inet {

namespace physicallayer {

Define_Module(LoRaDSMERadio);

void LoRaDSMERadio::initialize(int stage)
{
    LoRaRadio::initialize(stage);
    if (stage == INITSTAGE_LOCAL) {
        dutyCycle = registerSignal("dutyCycle");
        dutyCycleTimer = new cMessage("dutyCycleTimer");
        scheduleAt(simTime() + SimTime(DUTY_CYCLE_PERIOD), dutyCycleTimer);
    }
}

void LoRaDSMERadio::aggregateToA(simtime_t T)
{
    aggregateTime += T;
}

void LoRaDSMERadio::handleSelfMessage(cMessage *message)
{
    if(message == dutyCycleTimer) {
        scheduleAt(simTime() + DUTY_CYCLE_PERIOD, dutyCycleTimer);

        simtime_t dc = aggregateTime * 100 / SimTime(3600);
        emit(dutyCycle, dc);
        aggregateTime = 0;
    }
    else {
        LoRaRadio::handleSelfMessage(message);
    }
}

}
}

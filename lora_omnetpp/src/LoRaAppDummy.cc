#include "LoRaAppDummy.h"
#include "LoRaApp/SimpleLoRaApp.h"

namespace inet {

Define_Module(LoRaAppDummy);

void LoRaAppDummy::initialize(int stage)
{
    cSimpleModule::initialize(stage);
    if (stage == INITSTAGE_LOCAL) {
    }
    else if (stage == INITSTAGE_APPLICATION_LAYER) {
        //LoRa physical layer parameters
        loRaTP = par("initialLoRaTP").doubleValue();
        loRaCF = units::values::Hz(par("initialLoRaCF").doubleValue());
        loRaSF = par("initialLoRaSF");
        loRaBW = inet::units::values::Hz(par("initialLoRaBW").doubleValue());
        loRaCR = par("initialLoRaCR");
        loRaUseHeader = par("initialUseHeader");
        evaluateADRinNode = par("evaluateADRinNode");
        sfVector.setName("SF Vector");
        tpVector.setName("TP Vector");
    }
}

void LoRaAppDummy::finish()
{
}

} //end namespace inet

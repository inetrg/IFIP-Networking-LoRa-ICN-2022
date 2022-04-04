#ifndef LORAAPPDUMMY_H_
#define LORAAPPDUMMY_H_

#include <omnetpp.h>
#include "LoRaApp/SimpleLoRaApp.h"
using namespace omnetpp;

namespace inet {

/**
 * TODO - Generated class
 */
class INET_API LoRaAppDummy : public SimpleLoRaApp
{
    protected:
        void initialize(int stage) override;
        void finish() override;
    public:
        LoRaAppDummy() {}
        void setCF(Hz CF) {
            loRaCF = CF;
        }
};

}

#endif

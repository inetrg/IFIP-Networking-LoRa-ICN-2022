//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU Lesser General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
// 
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU Lesser General Public License for more details.
// 
// You should have received a copy of the GNU Lesser General Public License
// along with this program.  If not, see http://www.gnu.org/licenses/.
// 

#ifndef LORADSMETRANSMITTER_H_
#define LORADSMETRANSMITTER_H_

#include <omnetpp.h>
#include "inet/physicallayer/wireless/common/base/packetlevel/FlatTransmitterBase.h"
#include "LoRaPhy/LoRaModulation.h"
#include "LoRaPhy/LoRaTransmission.h"
#include "LoRa/LoRaRadio.h"
#include "LoRa/LoRaMacFrame_m.h"

namespace inet {

namespace physicallayer {

class LoRaDSMETransmitter : public FlatTransmitterBase {
    public:
        LoRaDSMETransmitter();
        virtual void initialize(int stage) override;
        virtual std::ostream& printToStream(std::ostream& stream, int level, int evFlags = 0) const override;
        virtual const ITransmission *createTransmission(const IRadio *radio, const Packet *packet, const simtime_t startTime) const override;

    private:

        bool iAmGateway;

        simsignal_t LoRaTransmissionCreated;

};

}

}

#endif /* LORADSMETRANSMITTER_H_ */

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

#include "LoRaDSMEReceiver.h"
#include "LoRaPhy/LoRaPhyPreamble_m.h"
//#include "inet/physicallayer/wireless/common/analogmodel/packetlevel/ScalarNoise.h"
//#include "inet/physicallayer/wireless/common/contract/packetlevel/SignalTag_m.h"

namespace inet {

namespace physicallayer {

Define_Module(LoRaDSMEReceiver);

bool LoRaDSMEReceiver::computeIsReceptionAttempted(const IListening *listening, const IReception *reception, IRadioSignal::SignalPart part, const IInterference *interference) const
{
    if(isPacketCollided(reception, part, interference))
    {
        auto packet = reception->getTransmission()->getPacket();
        const auto &chunk = packet->peekAtFront<FieldsChunk>();
        auto loraMac = dynamicPtrCast<const LoRaMacFrame>(chunk);
        auto loraPreamble = dynamicPtrCast<const LoRaPhyPreamble>(chunk);
        MacAddress rec;
        if (loraPreamble)
            rec = loraPreamble->getReceiverAddress();
        else if (loraMac)
            rec = loraMac->getReceiverAddress();

        if (iAmGateway == false) {
            /* NOTE: This removes the dependency between the LoRa PHY and the
             * MAC Layer
             */
#if 0
            /* TODO: Complete */
            auto *macLayer = check_and_cast<LoRaMac *>(getParentModule()->getParentModule()->getSubmodule("mac"));
            if (rec == macLayer->getAddress()) {
                const_cast<LoRaReceiver* >(this)->numCollisions++;
            }
#endif
            //EV << "Node: Extracted macFrame = " << loraMacFrame->getReceiverAddress() << ", node address = " << macLayer->getAddress() << std::endl;
        } else {
            auto *gwMacLayer = check_and_cast<LoRaGWMac *>(getParentModule()->getParentModule()->getSubmodule("mac"));
            EV << "GW: Extracted macFrame = " << rec << ", node address = " << gwMacLayer->getAddress() << std::endl;
            if (rec == MacAddress::BROADCAST_ADDRESS) {
                const_cast<LoRaDSMEReceiver* >(this)->numCollisions++;
            }
        }
        return false;
    } else {
        return true;
    }
}


}
}

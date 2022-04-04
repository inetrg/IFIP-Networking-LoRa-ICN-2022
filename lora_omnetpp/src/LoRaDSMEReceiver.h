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

#ifndef LORADSMERECEIVER_H
#define LORADSMERECEIVER_H

#include "LoRaPhy/LoRaReceiver.h"
#include "inet/physicallayer/wireless/common/contract/packetlevel/IRadioMedium.h"
#include "inet/physicallayer/wireless/common/radio/packetlevel/ReceptionResult.h"
#include "inet/physicallayer/wireless/common/radio/packetlevel/BandListening.h"
#include "inet/physicallayer/wireless/common/radio/packetlevel/ListeningDecision.h"
#include "inet/physicallayer/wireless/common/radio/packetlevel/ReceptionDecision.h"
#include "inet/physicallayer/wireless/common/base/packetlevel/NarrowbandNoiseBase.h"
#include "inet/physicallayer/wireless/common/analogmodel/packetlevel/ScalarSnir.h"
#include "inet/physicallayer/wireless/common/base/packetlevel/FlatReceiverBase.h"
#include "LoRaPhy/LoRaModulation.h"
#include "LoRaPhy/LoRaTransmission.h"
#include "LoRaPhy/LoRaReception.h"
#include "LoRaPhy/LoRaBandListening.h"
#include "LoRa/LoRaRadio.h"
#include "LoRaApp/SimpleLoRaApp.h"
#include "LoRa/LoRaMac.h"
#include "LoRa/LoRaGWMac.h"


//based on Ieee802154UWBIRReceiver

namespace inet {

namespace physicallayer {

class INET_API LoRaDSMEReceiver : public LoRaReceiver
{
public:
  virtual bool computeIsReceptionAttempted(const IListening *listening, const IReception *reception, IRadioSignal::SignalPart part, const IInterference *interference) const override;

};

}

}

#endif /* LORADSMERECEIVER_H */

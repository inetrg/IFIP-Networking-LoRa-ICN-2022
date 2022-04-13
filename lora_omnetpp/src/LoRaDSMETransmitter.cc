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

#include "LoRaDSMETransmitter.h"
#include "LoRaDSMERadio.h"
#include "inet/physicallayer/wireless/common/analogmodel/packetlevel/ScalarTransmission.h"
#include "LoRaPhy/LoRaModulation.h"
#include "LoRaPhy/LoRaPhyPreamble_m.h"
#include <algorithm>


namespace inet {

namespace physicallayer {

Define_Module(LoRaDSMETransmitter);

LoRaDSMETransmitter::LoRaDSMETransmitter() :
    FlatTransmitterBase()
{
}

void LoRaDSMETransmitter::initialize(int stage)
{
    TransmitterBase::initialize(stage);
    if (stage == INITSTAGE_LOCAL) {
        preambleDuration = 0.001; //par("preambleDuration");
        headerLength = b(par("headerLength"));
        bitrate = bps(par("bitrate"));
        power = W(par("power"));
        centerFrequency = Hz(par("centerFrequency"));
        bandwidth = Hz(par("bandwidth"));
        LoRaTransmissionCreated = registerSignal("LoRaTransmissionCreated");

        if(strcmp(getParentModule()->getClassName(), "inet::physicallayer::LoRaGWRadio") == 0)
        {
            iAmGateway = true;
        } else iAmGateway = false;
    }
}

std::ostream& LoRaDSMETransmitter::printToStream(std::ostream& stream, int level, int evFlags) const
{
    stream << "LoRaDSMETransmitter";
    return FlatTransmitterBase::printToStream(stream, level, evFlags);
}

const ITransmission *LoRaDSMETransmitter::createTransmission(const IRadio *transmitter, const Packet *macFrame, const simtime_t startTime) const
{
//    TransmissionBase *controlInfo = dynamic_cast<TransmissionBase *>(macFrame->getControlInfo());
    //W transmissionPower = controlInfo && !std::isnan(controlInfo->getPower().get()) ? controlInfo->getPower() : power;
//    const LoRaMacFrame *frame = check_and_cast<const LoRaMacFrame *>(macFrame);
    EV << macFrame->getDetailStringRepresentation(evFlags) << endl;
    const auto &frame = macFrame->peekAtFront<LoRaPhyPreamble>();

    int nPreamble = 8;
    simtime_t Tsym = (pow(2, frame->getSpreadFactor()))/(frame->getBandwidth().get()/1000);
    simtime_t Tpreamble = (nPreamble + 4.25) * Tsym / 1000;

    //preambleDuration = Tpreamble;
    int payloadBytes = B(macFrame->getTotalLength()).get();
    int payloadSymbNb = 8;
    payloadSymbNb += std::ceil((8*payloadBytes - 4*frame->getSpreadFactor() + 28 + 16 - 20*0)/(4*(frame->getSpreadFactor()-2*0)))*(frame->getCodeRendundance() + 4);
    if(payloadSymbNb < 8) payloadSymbNb = 8;
    simtime_t Theader = 0.5 * (8+payloadSymbNb) * Tsym / 1000;
    simtime_t Tpayload = 0.5 * (8+payloadSymbNb) * Tsym / 1000;

    const simtime_t duration = Tpreamble + Theader + Tpayload;
    const simtime_t endTime = startTime + duration;
    IMobility *mobility = transmitter->getAntenna()->getMobility();
    const Coord startPosition = mobility->getCurrentPosition();
    const Coord endPosition = mobility->getCurrentPosition();
    const Quaternion startOrientation = mobility->getCurrentAngularPosition();
    const Quaternion endOrientation = mobility->getCurrentAngularPosition();
    W transmissionPower = computeTransmissionPower(macFrame);
    LoRaDSMERadio *dsme_radio = check_and_cast<LoRaDSMERadio*>(getParentModule());
    dsme_radio->aggregateToA(duration);
    const_cast<LoRaDSMETransmitter* >(this)->emit(LoRaTransmissionCreated, duration);

    if(!iAmGateway) {
        LoRaRadio *radio = check_and_cast<LoRaRadio *>(getParentModule());
        radio->setCurrentTxPower(transmissionPower.get());
    }
    return new LoRaTransmission(transmitter,
            macFrame,
            startTime,
            endTime,
            Tpreamble,
            Theader,
            Tpayload,
            startPosition,
            endPosition,
            startOrientation,
            endOrientation,
            transmissionPower,
            frame->getCenterFrequency(),
            frame->getSpreadFactor(),
            frame->getBandwidth(),
            frame->getCodeRendundance());}

}

}

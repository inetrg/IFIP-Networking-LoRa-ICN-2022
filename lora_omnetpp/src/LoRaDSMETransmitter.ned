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

package lora_omnetpp;

import inet.physicallayer.wireless.common.base.packetlevel.NarrowbandTransmitterBase;

module LoRaDSMETransmitter extends NarrowbandTransmitterBase
{
    parameters:
        @signal[LoRaTransmissionCreated](type=simtime_t); // optional
        @statistic[LoRaTransmissionCreated](source=LoRaTransmissionCreated; record=vector);
        modulation = default("LoRaModulation");
        @class(inet::physicallayer::LoRaDSMETransmitter);
}

//
// Copyright (C) 2016 Florian Kauer <florian.meier@koalo.de>
// Copyright (C) 2017 Maximilian Köstler <maximilian.koestler@tuhh.de>
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

package lora.simulations;


import inet.linklayer.ieee802154.Ieee802154NarrowbandInterface;
import inet.node.inet.WirelessHost;
import inet.physicallayer.wireless.ieee802154.packetlevel.Ieee802154NarrowbandScalarRadio;

import inet_dsme.applications.generic.PRRTrafGen;
import inet.routing.gpsr.Gpsr;
import lora_omnetpp.LoRaAppDummy;

//
// A wireless host containing a WirelessInterface, GPSR and a PRRTrafGen.
//
module IEEE802154ExampleHost extends WirelessHost
{
    parameters:
        @display("i=device/accesspoint;bgb=,-");

        forwarding = default(true);

        hasIpv4 = false;
        hasIpv6 = false;
        hasGn   = true;

        hasUdp = true;
        hasTcp = false;
        hasSctp = false;

	    bool hasGpsr = default(true);

        generic.typename = "NextHopNetworkLayer";
        wlan[*].typename = default("WirelessInterface");
        wlan[*].radio.typename = default("LoRaDSMERadio");

    submodules:
        gpsr: Gpsr if hasGpsr {
            parameters:
                @display("p=739,339");
                routingTableModule = "^.generic.routingTable";
                networkProtocolModule = "^.generic.nextHop";
				positionByteLength = 2 B; // assumes a 3D coordinate can be specified in 6 byte
                beaconInterval = 1250s;
                maxJitter = 5s;
                neighborValidityInterval = 30000s;
        }

        trafficgen: PRRTrafGen {
            parameters:
                @display("p=188,287");
                protocol     = default(150);
                packetLength = default(10B);
        }

        SimpleLoRaApp: LoRaAppDummy {
            @display("p=137,32");
        }

    connections allowunconnected:
        trafficgen.ipOut --> tn.in++;
        trafficgen.ipIn <-- tn.out++;

        gpsr.ipOut --> tn.in++ if hasGpsr;
        gpsr.ipIn <-- tn.out++ if hasGpsr;
}

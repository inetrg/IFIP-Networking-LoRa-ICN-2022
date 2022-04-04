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

package ccnsim_dsme;


import inet.linklayer.ieee802154.Ieee802154NarrowbandInterface;
import inet.node.inet.WirelessHost;
import inet.physicallayer.wireless.ieee802154.packetlevel.Ieee802154NarrowbandScalarRadio;

import inet_dsme.applications.generic.PRRTrafGen;
import inet.routing.gpsr.Gpsr;

import modules.node.node;
import lora_omnetpp.LoRaAppDummy;

//
// A wireless host containing a WirelessInterface, GPSR and a PRRTrafGen.
//
module ccnsim_dsme_host extends WirelessHost
{
    parameters:
        @display("i=device/accesspoint;bgb=,-");

        forwarding = default(true);

        hasIpv4 = false;
        hasIpv6 = false;
        hasGn   = false;

        hasUdp = false;
        hasTcp = false;
        hasSctp = false;

	    bool hasGpsr = default(false);

        generic.typename = "NextHopNetworkLayer";
        wlan[*].typename = default("DSMENic");

    gates:
        inout data_face;

    submodules:
            ccnsim_to_inet: ccnsim_to_inet {

            }

    // submodules:
    //     gpsr: Gpsr if hasGpsr {
    //         parameters:
    //             @display("p=739,339");
    //             routingTableModule = "^.generic.routingTable";
    //             networkProtocolModule = "^.generic.nextHop";
	// 			positionByteLength = 2 B; // assumes a 3D coordinate can be specified in 6 byte
    //             beaconInterval = 10s;
    //             maxJitter = 5s;
    //             neighborValidityInterval = 300s;
    //     }

    //     trafficgen: PRRTrafGen {
    //         parameters:
    //             @display("p=188,287");
    //             protocol     = default(150);
    //             packetLength = default(10B);
    //     }

        SimpleLoRaApp: LoRaAppDummy {
            @display("p=137,32");
        }



    connections:
        // trafficgen.ipOut --> tn.in++;
        // trafficgen.ipIn <-- tn.out++;

        // out to out, in to in??? seems to fix...
        // data_face$i --> tn.in++;
        // data_face$o <-- tn.out++;

        data_face$o <-- ccnsim_to_inet.upper_layer$o;
        data_face$i --> ccnsim_to_inet.upper_layer$i;

        ccnsim_to_inet.lower_layer$o --> li.in++;
        ccnsim_to_inet.lower_layer$i <-- li.out++;

        // gpsr.ipOut --> tn.in++ if hasGpsr;
        // gpsr.ipIn <-- tn.out++ if hasGpsr;
}

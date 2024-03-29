//
// Copyright (C) 2016 Florian Kauer <florian.kauer@koalo.de>
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
//

package lora.simulations;

import inet.networklayer.configurator.nexthop.NextHopNetworkConfigurator;
import inet.physicallayer.wireless.ieee802154.packetlevel.Ieee802154NarrowbandScalarRadioMedium;
import loranetwork.LoRaPhy.LoRaMedium;

network Net802154
{
    parameters:
        int numHosts;

    submodules:
        host[numHosts]: IEEE802154ExampleHost {
            parameters:
                @display("r=,,#707070;p=250,58;is=vs");
        }

        radioMedium: LoRaMedium {
            parameters:
                @display("p=60,50");
                mediumLimitCache.maxTransmissionDuration = 4s;
                backgroundNoise.typename = "";
        }

	    configurator: NextHopNetworkConfigurator {
	    }
}

package ccnsim_dsme.simulations;
import ccnsim_dsme.ccnsim_dsme_host;

import inet.networklayer.configurator.nexthop.NextHopNetworkConfigurator;
import inet.physicallayer.wireless.ieee802154.packetlevel.Ieee802154NarrowbandScalarRadioMedium;
import inet.physicallayer.wireless.common.contract.packetlevel.IRadioMedium;

// import inet_dsme.simulations.IEEE802154ExampleHost;

network ccnsim_dsme_network extends star_network{
    parameters:
        int numHosts;


    submodules:
        radioMedium: <default("Ieee802154NarrowbandScalarRadioMedium")> like IRadioMedium {
            parameters:
                @display("p=60,50");
        }

    connections:
        //for i = 0..n-1{
	    //    node[i].data_face <--> { delay = 0ms; } <--> host[i].data_face;
	    //}
        // node[0].data_face <--> {delay = 0ms;} <--> host[0].data_face;
        // node[1].data_face <--> {delay = 0ms;} <--> host[1].data_face;
        // node[2].data_face <--> {delay = 0ms;} <--> host[2].data_face;
        // node[3].data_face <--> {delay = 0ms;} <--> host[3].data_face;
        // node[4].data_face <--> {delay = 0ms;} <--> host[4].data_face;
        // node[5].data_face <--> {delay = 0ms;} <--> host[5].data_face;
        // node[6].data_face <--> {delay = 0ms;} <--> host[6].data_face;
        // node[7].data_face <--> {delay = 0ms;} <--> host[7].data_face;
        // node[8].data_face <--> {delay = 0ms;} <--> host[8].data_face;
}

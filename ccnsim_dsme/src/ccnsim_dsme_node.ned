package ccnsim_dsme;

// import inet_dsme.simulations.IEEE802154ExampleHost;
import inet.node.inet.WirelessHost;

import modules.node.strategy.*;
import modules.node.cache.*;
import modules.node.Inode;
import inet.node.inet.WirelessHost;
import lora_omnetpp.LoRaAppDummy;

module ccnsim_dsme_node extends WirelessHost like Inode{
    parameters:
	string RS = default("lru_cache");
	string FS = default("spr");

	int level = default (-1);
	double betweenness = default (-1);
	content_store.DS = default("lce");
        forwarding = default(false);

        hasIpv4 = false;
        hasIpv6 = false;
        hasGn   = false;

        hasUdp = false;
        hasTcp = false;
        hasSctp = false;

	    bool hasGpsr = default(false);


        numWlanInterfaces = default(1);
        wlan[*].typename = default("DSMENic");


	@display("i=abstract/router;is=s");
    gates:
	inout face[];
	inout client_port;

    submodules:
	core_layer : ccnsim_dsme_core_layer;
	strategy_layer : < "modules.node.strategy." + FS > like strategy;
	content_store : < "modules.node.cache." + RS > like cache;
	face_dsme : face_dsme;
        SimpleLoRaApp: LoRaAppDummy {
            @display("p=137,32");
        }
    ccnsim_to_inet: ccnsim_to_inet if numWlanInterfaces > 0 {

    }

	connections allowunconnected:
	// PK: keep connection of face[0] for client
	core_layer.face++ <--> face[0];

	for i=1..sizeof(face)-1{
		face_dsme.face++ <--> face [i];
		core_layer.face++ <--> face_dsme.upper_layer++;
		// face_dsme.lower_layer++ <--> face [i];
	}

	face_dsme.lower_layer <--> ccnsim_to_inet.upper_layer;
    ccnsim_to_inet.lower_layer$o --> li.in++;
    ccnsim_to_inet.lower_layer$i <-- li.out++;

	core_layer.client_port <--> client_port;
	core_layer.cache_port  <--> content_store.cache_port;
	core_layer.strategy_port <--> strategy_layer.strategy_port;
}

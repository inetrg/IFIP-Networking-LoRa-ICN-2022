package ccnsim_dsme;

import loranetwork.LoRaPhy.LoRaMedium;

module LoRaDSMEMedium extends LoRaMedium
{
    parameters:
        @display("p=60,50");
        mediumLimitCache.maxTransmissionDuration = 4s;
        backgroundNoise.typename = "";
}

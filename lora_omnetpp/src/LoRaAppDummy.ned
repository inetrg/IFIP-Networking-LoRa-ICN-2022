package lora_omnetpp;

import loranetwork.LoRaApp.SimpleLoRaApp;

module LoRaAppDummy extends SimpleLoRaApp
{
    parameters:
        numberOfPacketsToSend = 0;
        timeToFirstPacket = 1000s;
        initialLoRaSF = 7;
        @class(inet::LoRaAppDummy);
}

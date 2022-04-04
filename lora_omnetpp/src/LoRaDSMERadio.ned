package lora_omnetpp;

import loranetwork.LoRa.LoRaRadio;

module LoRaDSMERadio extends LoRaRadio
{
    parameters:
        @signal[dutyCycle](type=simtime_t); // optional
        @statistic[dutyCycle](source=dutyCycle; record=min,max,avg);
        transmitter.typename = "LoRaDSMETransmitter";
        transmitter.headerLength = 0B;

        receiver.typename = "LoRaDSMEReceiver";

    @class(inet::physicallayer::LoRaDSMERadio);
}

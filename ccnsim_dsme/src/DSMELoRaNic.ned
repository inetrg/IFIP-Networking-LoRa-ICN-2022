package ccnsim_dsme;

import inet.linklayer.common.WirelessInterface;

module DSMELoRaNic extends WirelessInterface
{
    mac.typename = "DSMELoRa";
    radio.typename = "LoRaDSMERadio";
}

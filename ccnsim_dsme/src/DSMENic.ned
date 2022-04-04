package ccnsim_dsme;

import inet.linklayer.common.WirelessInterface;

module DSMENic extends WirelessInterface
{
    mac.typename = "DSME";
    radio.typename = "Ieee802154NarrowbandScalarRadio";
}

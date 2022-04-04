package lora_omnetpp;

import inet_dsme.DSME;

simple DSMELoRa extends DSME
{
    @signal[sentL2CCNFrame](type=unsigned long);
    @signal[txSuperframeID](type=unsigned int);
    @signal[txSlotID](type=unsigned int);
    @signal[rxSuperframeID](type=unsigned int);
    @signal[rxSlotID](type=unsigned int);
    @statistic[sentL2CCNFrame](source=sentL2CCNFrame; record=count, vector);
    bool CAPAckRequest;
    macCapReduction = default(false);
    bool gtsTx = default(false);
    bool gtsRx = default(false);
    @class(dsme::DSMELoRaPlatform);
}

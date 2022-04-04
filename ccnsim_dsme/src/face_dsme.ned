package ccnsim_dsme;

simple face_dsme
{
    parameters:
        @signal[SentCCNData](type=unsigned long);
        @signal[SentCCNInterest](type=unsigned long);
        @signal[SentCCNPush](type=unsigned long);
        @signal[SentCCNIndication](type=unsigned long);
        @signal[ReceivedCCNData](type=unsigned long);
        @signal[ReceivedCCNInterest](type=unsigned long);
        @signal[ReceivedCCNPush](type=unsigned long);
        @signal[ReceivedCCNIndication](type=unsigned long);

        @statistic[SentCCNData](source=SentCCNData; record=count, vector);
        @statistic[SentCCNInterest](source=SentCCNInterest; record=count, vector);
        @statistic[SentCCNIndication](source=SentCCNIndication; record=count, vector);
        @statistic[SentCCNPush](source=SentCCNPush; record=count, vector);
        @statistic[ReceivedCCNData](source=ReceivedCCNData; record=count, vector);
        @statistic[ReceivedCCNInterest](source=ReceivedCCNInterest; record=count, vector);
        @statistic[ReceivedCCNIndication](source=ReceivedCCNIndication; record=count, vector);
        @statistic[ReceivedCCNPush](source=ReceivedCCNPush; record=count, vector);
        
        bool interestCFP = default(false);
        bool pushCFP = default(false);
        bool dataCFP = default(false);
        bool indicationCFP = default(false);
        
        bool aggregateCCNData = default(false);

        @display("i=block/routing");

    gates:
        inout upper_layer[];
        inout face[];
        inout lower_layer;
}

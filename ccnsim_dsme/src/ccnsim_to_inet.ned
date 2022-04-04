package ccnsim_dsme;

simple ccnsim_to_inet
{
    parameters:
        @display("i=block/routing");

    @class (ccnsim_to_inet);

    gates:
        inout upper_layer;
        inout lower_layer;
}

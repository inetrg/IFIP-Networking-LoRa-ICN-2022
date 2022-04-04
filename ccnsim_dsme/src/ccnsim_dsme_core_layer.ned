package ccnsim_dsme;
import modules.node.core_layer;

module ccnsim_dsme_core_layer extends core_layer {
    volatile double timeToNext = default(35);
    volatile double warmUpTime @unit(s) = default(2000s);
    string distribution;
    string rfd_protocol;
    @class(ccnsim_dsme_core_layer);
}

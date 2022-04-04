.PHONY: all

all: gen-libs
	$(MAKE) -C ccnsim_dsme makefiles
	$(MAKE) -C ccnsim_dsme all -j3

gen-flora: inet
	$(MAKE) -C flora makefiles-static-lib
	$(MAKE) -C flora all -j3

gen-ccnsim:
	$(MAKE) -C ccnSim-0.4 makefiles-static-lib
	$(MAKE) -C ccnSim-0.4 all -j3

gen-lora-dsme: inet
	$(MAKE) -C inet-dsme makefiles-static-lib
	$(MAKE) -C inet-dsme all MODE=lora_omnetpp -j3

gen-lora_omnetpp: inet gen-flora gen-lora-dsme
	$(MAKE) -C lora_omnetpp makefiles-lib
	$(MAKE) -C lora_omnetpp all -j3

inet:
	$(MAKE) -C inet makefiles
	$(MAKE) -C inet -j3

gen-libs: gen-flora gen-ccnsim gen-lora-dsme gen-lora_omnetpp

clean:
	$(MAKE) -C lora_omnetpp cleanall
	$(MAKE) -C ccnSim-0.4 cleanall
	$(MAKE) -C inet-dsme cleanall
	$(MAKE) -C flora cleanall

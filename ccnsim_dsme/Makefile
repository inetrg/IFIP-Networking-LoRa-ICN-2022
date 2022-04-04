CCNSIM_DSME ?= ../src/$(shell basename $(CURDIR))
INET_PATH ?= ../../inet
FLORA_PATH ?= ../../flora
INET_DSME_PATH ?= ../../inet-dsme
CCNSIM_PATH ?= ../../ccnSim-0.4
LORA_OMNETPP_PATH ?= ../../lora_omnetpp
DATA_PATH ?= ../../data
SIMULATION ?= rfd_repos
CONFIG ?= INDICATION
RESULTS_PATH ?= results
DATA_FILE ?= $(shell date +data_%Y-%m-%d-%H.%M.%S.csv)
NUM_CORES ?= 4

LIBINET_PATH ?= $(INET_PATH)/src/INET
LIBLORA_OMNETPP_PATH ?= $(LORA_OMNETPP_PATH)/src/lora_omnetpp

OPP_MAKEMAKE_KARGS = -KINET_PROJ=$(INET_PATH) -KFLORA_PROJ=$(FLORA_PATH) -KINET_DSME_PROJ=$(INET_DSME_PATH) -KCCNSIM_PROJ=$(CCNSIM_PATH) -KLORA_OMNETPP_PROJ=$(LORA_OMNETPP_PATH) -KOMNETPP_CONFIGFILE=../../common/Makefile.inc
OPP_MAKEMAKE_DARGS = -DINET_IMPORT
OPP_MAKEMAKE_IARGS = -I. -I$$\(INET_PROJ\)/src -I. -I$$\(FLORA_PROJ\)/src -I$$\(INET_DSME_PROJ\)/src -I$$\(CCNSIM_PROJ\)/include -I$$\(CCNSIM_PROJ\)/src/packets -I$$\(LORA_OMNETPP_PROJ\)/src
OPP_MAKEMAKE_LARGS = -L$$\(INET_PROJ\)/src -L$$\(FLORA_PROJ\)/src -L$$\(INET_DSME_PROJ\)/src -L$$\(CCNSIM_PROJ\)/src
OPP_MAKEMAKE_DLARGS = -lINET$$\(D\) -lccnSim$$\(D\)# extend later

OPP_MAKEMAKE_ARGS += -f --deep
OPP_MAKEMAKE_ARGS += $(OPP_MAKEMAKE_KARGS)
OPP_MAKEMAKE_ARGS += $(OPP_MAKEMAKE_DARGS)
OPP_MAKEMAKE_ARGS += $(OPP_MAKEMAKE_IARGS)
OPP_MAKEMAKE_ARGS += $(OPP_MAKEMAKE_LARGS)
OPP_MAKEMAKE_ARGS += $(OPP_MAKEMAKE_DLARGS)

NED_INCLUDES = ../src:.:$(INET_PATH)/src:$(INET_PATH)/examples:$(INET_PATH)/tutorials:$(INET_PATH)/showcases:$(INET_DSME_PATH)/src:$(INET_DSME_PATH)/simulations:$(FLORA_PATH)/src:$(CCNSIM_PATH)/:$(LORA_OMNETPP_PATH)/src
# modules:$(CCNSIM_PATH)/modules/clients:$(CCNSIM_PATH)/networks
OMNETPP_LIBS += -l $(LIBINET_PATH) -l $(LIBLORA_OMNETPP_PATH)

CMDENV ?= 0
VERBOSE ?= 0

ifneq (,$(RUN))
  RUNARGS = -r $(RUN)
endif

ifneq (0, $(CMDENV))
  OMNETPP_EXTRA_ARGS += -u Cmdenv
  ifneq (0, $(VERBOSE))
	OMNETPP_EXTRA_ARGS += --cmdenv-express-mode=false --cmdenv-log-prefix="[%l] %m: %|"
  endif
endif

define CCNSIM_DSME_ARGS
-m -n $(NED_INCLUDES) --image-path=$(INET_PATH)/images $(OMNETPP_LIBS) $(OMNETPP_EXTRA_ARGS) $(SIMULATION).ini -c $(CONFIG) $(RUNARGS)
endef

all: checkmakefiles
	cd src && $(MAKE)

clean: checkmakefiles
	cd src && $(MAKE) clean

cleanall: checkmakefiles
	cd src && $(MAKE) MODE=release clean
	cd src && $(MAKE) MODE=debug clean
	rm -f src/Makefile

makefiles:
	cd src && opp_makemake $(OPP_MAKEMAKE_ARGS)


checkmakefiles:
	@if [ ! -f src/Makefile ]; then \
	echo; \
	echo '======================================================================='; \
	echo 'src/Makefile does not exist. Please use "make makefiles" to generate it!'; \
	echo '======================================================================='; \
	echo; \
	exit 1; \
	fi

configs:
	cd simulations && $(CCNSIM_DSME) $(CCNSIM_DSME_ARGS) -q runs

run:
	cd simulations && $(CCNSIM_DSME) $(CCNSIM_DSME_ARGS)

runall:
	cd simulations && opp_runall -j$(NUM_CORES) -b1 $(CCNSIM_DSME) $(CCNSIM_DSME_ARGS)

debug:
	cd simulations && $(CCNSIM_DSME)_dbg $(CCNSIM_DSME_ARGS)

gdb:
	cd simulations && gdb --args $(CCNSIM_DSME)_dbg $(CCNSIM_DSME_ARGS)

scavetool:
	cd simulations && opp_scavetool x $(RESULTS_PATH)/*.sca $(RESULTS_PATH)/*.vec -o $(DATA_PATH)/$(DATA_FILE)

LORA_OMNETPP ?= ../src/$(shell basename $(CURDIR))
INET_PATH ?= ../../inet
FLORA_PATH ?= ../../flora
INET_DSME_PATH ?= ../../inet-dsme
CCNSIM_DSME_PATH ?= ../../ccnsim_dsme
SIMULATION ?= example
CONFIG ?= DSME

OPP_MAKEMAKE_KARGS = -KINET_PROJ=$(INET_PATH) -KFLORA_PROJ=$(FLORA_PATH) -KINET_DSME_PROJ=$(INET_DSME_PATH) -KCCNSIM_DSME_PROJ=$(CCNSIM_DSME_PATH) -KCFLAGS_EXTRA=-DLORA_SYMBOL_TIME -KOMNETPP_CONFIGFILE=../../common/Makefile.inc
OPP_MAKEMAKE_DARGS = -DINET_IMPORT
OPP_MAKEMAKE_IARGS = -I. -I$$\(INET_PROJ\)/src -I. -I$$\(FLORA_PROJ\)/src -I$$\(INET_DSME_PROJ\)/src -I$$\(CCNSIM_DSME_PROJ\)/src
OPP_MAKEMAKE_LARGS = -L$$\(INET_PROJ\)/src -L$$\(FLORA_PROJ\)/src -L$$\(INET_DSME_PROJ\)/src
OPP_MAKEMAKE_DLARGS = -lINET$$\(D\) -lflora$$\(D\) -linet-dsme_lora_omnetpp$$\(D\)

OPP_MAKEMAKE_ARGS += -f --deep
OPP_MAKEMAKE_ARGS += $(OPP_MAKEMAKE_KARGS)
OPP_MAKEMAKE_ARGS += $(OPP_MAKEMAKE_DARGS)
OPP_MAKEMAKE_ARGS += $(OPP_MAKEMAKE_IARGS)
OPP_MAKEMAKE_ARGS += $(OPP_MAKEMAKE_LARGS)
OPP_MAKEMAKE_ARGS += $(OPP_MAKEMAKE_DLARGS)

INET_LIB = $(INET_PATH)/src/INET

NED_INCLUDES = ../src:.:$(INET_PATH)/src:$(INET_PATH)/examples:$(INET_PATH)/tutorials:$(INET_PATH)/showcases:$(INET_DSME_PATH)/src:$(FLORA_PATH)/src
OMNETPP_LIBS += -l  $(INET_LIB)

CMDENV ?= 0
VERBOSE ?= 0

ifneq (0, $(CMDENV))
  OMNETPP_EXTRA_ARGS += -u Cmdenv
  ifneq (0, $(VERBOSE))
	OMNETPP_EXTRA_ARGS += --cmdenv-express-mode=false --cmdenv-log-prefix="[%l] %m: %|"
  endif
endif

define LORA_OMNETPP_ARGS
-m -n $(NED_INCLUDES) --image-path=$(INET_PATH)/images $(OMNETPP_LIBS) $(OMNETPP_EXTRA_ARGS)
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

makefiles-lib:
	cd src && opp_makemake $(OPP_MAKEMAKE_ARGS) -s


checkmakefiles:
	@if [ ! -f src/Makefile ]; then \
	echo; \
	echo '======================================================================='; \
	echo 'src/Makefile does not exist. Please use "make makefiles" to generate it!'; \
	echo '======================================================================='; \
	echo; \
	exit 1; \
	fi

run:
	cd simulations && $(LORA_OMNETPP) $(LORA_OMNETPP_ARGS) $(SIMULATION).ini -c $(CONFIG)

debug:
	cd simulations && $(LORA_OMNETPP)_dbg $(LORA_OMNETPP_ARGS) $(SIMULATION).ini -c $(CONFIG)

gdb:
	cd simulations && gdb --args $(LORA_OMNETPP)_dbg $(LORA_OMNETPP_ARGS) $(SIMULATION).ini -c $(CONFIG)

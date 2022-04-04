LoRa sensor node for Omnet++
============================

## Instructions

1. Install Omnet++ 6 (Preview 10) from https://omnetpp.org/download/preview

Note: In some Linux distrubutions it might be necessary to set the
`LD_LIBRARY_PATH` to `<omnetpp_dir>/lib` in case `opp_*` commands fail
to link against Omnet++ libraries.

2. Update all git submodules
```
git submodule update --init --recursive
```

3. Build the `inet` framework:
```
make -C inet makefiles
make -C inet -j4
```

This version points to INET v4.3.0

5. Build `flora` (both standalone and libraries):
```
make -C flora makefiles
make -C flora -j4

BUILD_LIB=1 BUILD_STATIC_LIB=1 make -C flora makefiles
make -C flora -j4
```

6. Build `inet-dsme` (both standalone and shared library):
```
make -C inet-dsme makefiles
make -C inet-dsme -j4

BUILD_LIB=1 make -C inet-dsme makefiles
make -C inet-dsme -j4
```

7. Build `lora_omnetpp`:
```
make -C lora_omnetpp makefiles
make -C lora_omnetpp -j4
```

8. Run the simulation with:
```
make -C lora_omnetpp run
```

## Use an existing INET library instance

```
INET_PATH=<path_to_inet> make ...
```

This works for `flora` ,`inet-dsme` and `lora_omnetpp`

## Choose a different simulation

```
SIMULATION=<simulation> CONFIG=<config> make -C lora_omnetpp run
```

Simulations are a ini file stored in `lora_omnetpp/simulations`.
By default, `SIMULATION=omnetpp` (points to `simulations/omnetpp.ini`)

Configurations are sections of the INI file that describe a preset of parameters.
By default, `CONFIG=DSME` (points to `[DSME]` in `simulations/omnetpp.ini`)

## Use command line simulation instead of Qt/Tk based

```
CMDENV=1 make -C lora_omnetpp run
```

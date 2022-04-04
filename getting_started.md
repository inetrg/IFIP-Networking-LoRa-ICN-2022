

# Organization of this Folder

The most relevant contributions of our simulator are situated in different git submodules and folders:

- [inet](https://github.com/inetrg/inet/tree/6498d2423c72724269b8d7a943773ca11e62c505): Contains the OMNeT++ model for wireless and mobile networks.
- [ccnSim-0.4](https://github.com/inetrg/ccnSim-0.4/tree/fe0964e1162c5dc9303e622f461ff2c62bb0585e): Contains the ICN stack and our protocol extensions in *ccnSim*.
- [inet-dsme](https://github.com/inetrg/inet-dsme/tree/3c2483ad5afa3487e78f100c05f129e769363b44): Contains the INET adaption of the IEEE 802.15.4 DSME in *openDSME*
- [flora](https://github.com/inetrg/flora/tree/8126aafbeeeae6ea3d726e802b845cb83f3c7fa2): Contains the radio implementation of *FLoRa* (Framework for LoRa).
- [lora_omnetpp](lora_omnetpp/): Contains our DSME-to-LoRa adaptation.
- [ccnsim_dsme](ccnsim_dsme/): Contains our ICN-to-DSME adaptation.


# Prerequisites

All tools are available in a docker container, hence, only [docker](https://docker-docs.netlify.app/install/) needs to be installed on your system.


# Build Simulation Environment with Docker
1. Clone this repository and navigate to the root directory:

```
git clone https://github.com/inetrg/IFIP-Networking-LoRa-ICN-2022.git && cd IFIP-Networking-LoRa-ICN-2022
```

2. Update all git submodules:
```
git submodule update --init --recursive
```

3. Build `inetrg/ccnsim_dsme` container (takes some minutes):
```
docker build -t inetrg/ccnsim_dsme .
```

# Quick Start – All Simulation Scenarios
To generate all simulations, run the following command. Note that this may take hours to execute all configurations. The collected data will be stored under `data/`.
```
docker run --rm -it -v "$(pwd)/data:/root/data" inetrg/ccnsim_dsme
```

To kill the simulation environment, open an other terminal and run the following command to retrieve the `<container_id>`:
```
docker ps
```

and to stop the container:
```
docker stop <container_id>
```


# Single Simulation Scenarios
We support two simulation (`SIMULATION`) scenarios each with different configurations (`CONFIG`) of the communication pattern.

## Configuration Options
### LoRa -> Internet (Repository on Nodes; [`rfd_repos`](ccnsim_dsme/simulations/rfd_repos.ini))
In this scenario LoRa nodes act as repositories and data flows to the gateway.
We define three configurations:
- **VANILLA**: Standard ICN request-response scheme. LoRa nodes transmit ICN Interests to the gateway.
- **PUSH**: LoRa nodes "push" ICN Data to the gateway.
- **INDICATION**: LoRa noes send a modified ICN Interest packet (indication) to to trigger an ICN Interest request from the gateway.

### Internet -> LoRa (Repository on Gateway; [`rfd_clients`](ccnsim_dsme/simulations/rfd_clients.ini))
In this scenario the gateway acts as repository and and data flows to the LoRa nodes.
We define two configurations:
- **VANILLA**: Standard ICN request-response scheme. Gateway transmits ICN Interests to LoRa nodes.
- **DATA_BROADCAST**: The gateway transmits a single aggregated ICN Data packet to a group of LoRa nodes.

## Run Single Simulation in Docker Environment

1. Enter the Docker container shell
```
docker run --rm -it -v "$(pwd)/data:/root/data" inetrg/ccnsim_dsme bash
```

2. Run simulation environment
```
SIMULATION=<simulation> CONFIG=<config> make -C ccnsim_dsme runall
```
This will produce results for all parameters combinations (time to next, number
of nodes, etc) for the given scenario.
For example, to produce all results for the PUSH configuration of the `rfd_repos`
scenario:
```
SIMULATION=rfd_repos CONFIG=PUSH make -C ccnsim_dsme runall
```

3. Collect all simulations with
```
make -C ccnsim_dsme scavetool
```
The collected data will be stored under `data/`.
See [Result Analysis with Python](https://docs.omnetpp.org/tutorials/pandas/) for
analysing results.


# Based on https://github.com/omnetpp/dockerfiles

# Build omnetpp
FROM omnetpp/omnetpp-base:u18.04 as base
LABEL maintainer="Rudolf Hornig <rudi@omnetpp.org>"

SHELL ["/bin/bash", "-c"] 

# first stage - build omnet
FROM base as omnetpp_builder
WORKDIR /root
RUN wget https://github.com/omnetpp/omnetpp/archive/refs/tags/omnetpp-6.0pre10.tar.gz \
         --referer=https://omnetpp.org/ -O omnetpp-src-core.tgz --progress=dot:giga && \
         tar xf omnetpp-src-core.tgz && rm omnetpp-src-core.tgz
RUN mv omnetpp-omnetpp-6.0pre10 omnetpp
WORKDIR /root/omnetpp
ENV PATH /root/omnetpp/bin:$PATH
# remove unused files and build
RUN cp configure.user.dist configure.user
RUN source setenv -f && ./configure WITH_QTENV=no WITH_OSG=no WITH_OSGEARTH=no && \
make -j $(nproc) MODE=release base && \
rm -r doc out test samples misc config.log config.status

# second stage - copy only the final binaries (to get rid of the 'out' folder and reduce the image size)
FROM base AS omnetpp_base
ENV OPP_VER=6.0pre10
RUN mkdir -p /root/omnetpp
WORKDIR /root/omnetpp
COPY --from=omnetpp_builder /root/omnetpp/ .
ENV PATH /root/omnetpp/bin:$PATH
RUN chmod 775 /root/ && \
    mkdir -p /root/models && \
    chmod 775 /root/models
WORKDIR /root/models
RUN echo 'PS1="omnetpp-omnetpp-6.0pre10:\w\$ "' >> /root/.bashrc && chmod +x /root/.bashrc && \
    touch /root/.hushlogin
ENV HOME=/root/

# Build INET
FROM omnetpp_base as builder

ADD inet /root/inet
WORKDIR /root/inet
RUN make makefiles && make -j $(nproc) && rm -rf out

# Build FLoRa
FROM omnetpp_base as flora

ADD common /root/common
WORKDIR /root/inet
COPY --from=builder /root/inet/ .
WORKDIR /root
ADD flora /root/flora
WORKDIR /root/flora
RUN make makefiles-static-lib && make -j$(nproc) && rm -rf out

# Build inet-dsme
FROM omnetpp_base as inet-dsme
ADD common /root/common
WORKDIR /root/inet
COPY --from=builder /root/inet/ .
WORKDIR /root
ADD inet-dsme /root/inet-dsme
WORKDIR /root/inet-dsme
RUN make makefiles-static-lib && make -j$(nproc) MODE=lora_omnetpp && rm -rf out

FROM omnetpp_base as ccnsim
ADD ccnSim-0.4 /root/ccnSim-0.4
WORKDIR /root/ccnSim-0.4
RUN apt-get update && apt-get install -y libboost-all-dev
ADD common /root/common
RUN make makefiles-static-lib && make -j$(nproc) && rm -rf out

# Build ccnsim_dsme
FROM ccnsim AS ccnsim_dsme
ADD common /root/common
WORKDIR /root/inet
COPY --from=builder /root/inet/ .
WORKDIR /root/flora
COPY --from=flora /root/flora/ .
WORKDIR /root/inet-dsme
COPY --from=inet-dsme /root/inet-dsme/ .
WORKDIR /root/ccnSim-0.4
COPY --from=ccnsim /root/ccnSim-0.4/ .

WORKDIR /root/
ADD ccnsim_dsme /root/ccnsim_dsme
WORKDIR /root/ccnsim_dsme
RUN WITH_QTENV=no make makefiles && make -j$(nproc) && rm -rf out

# Build lora_omnetpp
FROM omnetpp_base as lora_omnetpp
ADD common /root/common
ADD lora_omnetpp /root/lora_omnetpp
WORKDIR /root/inet
COPY --from=builder /root/inet/ .
WORKDIR /root/flora
COPY --from=flora /root/flora/ .
WORKDIR /root/inet-dsme
COPY --from=inet-dsme /root/inet-dsme/ .
WORKDIR /root/ccnsim_dsme
COPY --from=ccnsim_dsme /root/ccnsim_dsme/ .
WORKDIR /root/lora_omnetpp
RUN make makefiles-lib && make -j$(nproc) && rm -rf out

FROM omnetpp_base
WORKDIR /root/inet
COPY --from=builder /root/inet/ .
WORKDIR /root/flora
COPY --from=flora /root/flora/ .
WORKDIR /root/inet-dsme
COPY --from=inet-dsme /root/inet-dsme/ .
WORKDIR /root/ccnSim-0.4
COPY --from=ccnsim /root/ccnSim-0.4/ .
WORKDIR /root/ccnsim_dsme
COPY --from=ccnsim_dsme /root/ccnsim_dsme/ .
WORKDIR /root/lora_omnetpp
COPY --from=lora_omnetpp /root/lora_omnetpp/ .
WORKDIR /root
ADD runall .
CMD ["./runall"]

# Long-Range ICN for the IoT: Exploring a LoRa System Design (IFIP Networking 2022)

[![Paper][paper-badge]][paper-link]
[![Preprint][preprint-badge]][preprint-link]
[![Video][video-badge]][video-link]

This repository contains code and documentation to reproduce experimental results of the paper **"[Long-Range ICN for the IoT: Exploring a LoRa System Design][preprint-link]"** published in Proc. of the IFIP Networking Conference 2022.

* Peter Kietzmann, José Alamos, Dirk Kutscher, Thomas C. Schmidt, Matthias Wählisch,
**Long-Range ICN for the IoT: Exploring a LoRa System Design**,
In: Proc. of 21th IFIP Networking Conference, Piscataway, NJ, USA: IEEE, 2022.

 **Abstract**
 > This paper presents LoRa-ICN, a comprehensive IoT networking system based
on a common long-range communication layer (LoRa) combined with
Information-Centric Networking~(ICN) principles.  We have replaced the
LoRaWAN MAC layer with an IEEE 802.15.4 Deterministic and Synchronous
Multi-Channel Extension (DSME). This multifaceted  MAC layer allows for
different mappings of ICN message semantics, which we explore to enable
new LoRa scenarios.
We designed LoRa-ICN  from the ground-up to improve reliability
and to reduce dependency on centralized components in
LoRa IoT scenarios.
We have implemented a feature-complete prototype in a common network
simulator to validate our approach. Our results show design trade-offs
of different mapping alternatives in terms of robustness and~efficiency.

Please follow our [Getting Started](getting_started.md) instructions for further information how to compile and execute the code.

<!-- TODO: update URLs -->
[paper-link]:https://doi.org/10.23919/IFIPNetworking55013.2022.9829792
[preprint-link]:https://arxiv.org/pdf/2204.11040.pdf
[video-link]:https://youtu.be/tJRfUUsuqyQ
[paper-badge]: https://img.shields.io/badge/Paper-IEEE%20Xplore-green
[preprint-badge]: https://img.shields.io/badge/Preprint-arXiv-green
[video-badge]:https://img.shields.io/youtube/views/tJRfUUsuqyQ?style=social

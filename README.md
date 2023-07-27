# 5G Core Service Consumers

This is a set of service consumer libraries and testing tools for communicating with 5G Core Network Functions.

## Introduction

The 5G Core presents several Network Functions, each of which has its own set of service interfaces. This is a collection of reusable service consumer libraries designed to talk to the 5G Core Network Functions using some of these service interfaces.

### BSF discovery protocols

The Binding Service Function is responsible for maintaining a mapping of UE PDU Session to the PCF which is handling that PDU Session. The `libscbsf` library aids in discovery of the BSF in the 5G Core and subsequent lookup of a PCF using the IP address or the UE.

### PCF PolicyAuthorization protocol

The Policy Control Function is responsible for applying charging and network policy to the PDU sessions of UEs. The PolicyAuthorization protocol is used by an Application Function to request policy changes to the PDU session on behalf of the UE. This allows an Application Function to request particular QoS parameters for selected IP traffic flows within the PDU session, enabling the AF to implement background downloads or delivery boost features. The `libscpcf` library allows an application to connect to a PCF and request an `AppSessionContext` which it can then use to manipulate the network routing policies for traffic flowing across specific flows within a UE's PDU Session.

### Specifications

A list of specification related to this repository is available in the [Standards Wiki](https://github.com/5G-MAG/Standards/wiki/5G-Downlink-Media-Streaming-Architecture-(5GMSd):-Relevant-Specifications).

### About the implementation

These libraries and tools are based upon the [Open5GS](https://open5gs.org/) framework.

## Install dependencies

```bash
sudo apt install git ninja-build build-essential flex bison libsctp-dev libgnutls28-dev libgcrypt-dev libssl-dev libidn11-dev libmongoc-dev libbson-dev libyaml-dev libnghttp2-dev libmicrohttpd-dev libcurl4-gnutls-dev libnghttp2-dev libtins-dev libtalloc-dev meson
```

## Downloading

Release tar files can be downloaded from <https://github.com/5G-MAG/rt-5gc-service-consumers/releases>.

The source can be obtained by cloning the github repository.

For example to download the latest release you can use:

```bash
cd ~
git clone --recurse-submodules https://github.com/5G-MAG/rt-5gc-service-consumers.git
```

## Building

The build process requires a working Internet connection as project dependancies are downloaded during the build.

To build the libraries and tools from the source:

```bash
cd ~/rt-5gc-service-consumers
meson build
ninja -C build
```

## Installing

To install the built libraries and tools:

```bash
cd ~/rt-5gc-service-consumers
sudo meson install --no-rebuild
```

## Running

The PCF PolicyAuthorization tool can be run with a command like:

```bash
/usr/local/bin/pcf-policyauthorization -a 12.34.56.78 -n 98.76.54.32:1234
```

To get the full command help for the PCF PolicyAuthorization tool use the command:

```bash
/usr/local/bin/pcf-policyauthorization -h
```

## Development

This project follows
the [Gitflow workflow](https://www.atlassian.com/git/tutorials/comparing-workflows/gitflow-workflow). The
`development` branch of this project serves as an integration branch for new features. Consequently, please make sure to
switch to the `development` branch before starting the implementation of a new feature.


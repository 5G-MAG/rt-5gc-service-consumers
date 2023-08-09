# 5G Core Service Consumers

This is a set of service consumer libraries and testing tools for communicating with 5G Core Network Functions.

## Introduction

The 5G Core presents several Network Functions, each of which has its own set of service interfaces. This is a collection of reusable service consumer libraries designed to talk to the 5G Core Network Functions using some of these service interfaces.

In addition, command line tools are provided to demonstrate the use of these service consumer libraries.

### Relevant specifications

A list of specification related to this repository is available in the [Standards Wiki](https://github.com/5G-MAG/Standards/wiki/5G-Downlink-Media-Streaming-Architecture-(5GMSd):-Relevant-Specifications).

### About the implementation

These libraries and tools are based upon the [Open5GS](https://open5gs.org/) framework.

## Service consumer libraries

### `libscbsf` - Binding Support Function (BSF) service consumer library

The Binding Support Function (BSF) is responsible for maintaining a mapping between UE PDU Session and the PCF which is managing that PDU Session.

The `libscbsf` library aids in discovery of the BSF in the 5G Core (by interrogating the NRF) and subsequently looking up which PCF is managing the PDU Session for a UE, identified by its IP address.

This library implements the service consumer end of the following service-based APIs:
- *Nbsf_Management*

### `libscpcf` - Policy Control Function (PCF) service consumer library

The Policy Control Function (PCF) is responsible for applying charging and network policy to the PDU sessions of UEs. The *Npcf_PolicyAuthorization* service API is used at reference point N5 by an Application Function (AF) to request policy changes to the PDU session on behalf of the UE. This allows an Application Function to manipulate particular network QoS parameters for selected IP traffic flows within the PDU session.

The `libscpcf` library allows an application to connect to a PCF and request an `AppSessionContext` which it can then use to manipulate the network routing policies for traffic passing across specific application flows within a UE's PDU Session.

This library implements the service consumer end of the following service-based APIs:
- *Npcf_PolicyAuthorization*


## Command line tools

### `pcf-policyauthorization`

The `pcf-policyauthorization` tool manipulates the network Quality of Service parameters of Application Session Contexts in the PCF by using the **PCF service consumer library** to invoke operations on the *Npcf_PolicyAuthorization* service API.

The PCF address can be explicitly specified at the command line if this is already known. Alternatively, the tool can also use the **BSF service consumer library** to look up which PCF instance is managing the PDU Session of interest (based on the IP address of a UE registered with the AMF).


## Install dependencies

To build and use the service consumer libraries and accompanying command line tools, you will need to install the following packages:

```bash
sudo apt install git ninja-build build-essential flex bison libsctp-dev libgnutls28-dev libgcrypt-dev libssl-dev libidn11-dev libmongoc-dev libbson-dev libyaml-dev libnghttp2-dev libmicrohttpd-dev libcurl4-gnutls-dev libnghttp2-dev libtins-dev libtalloc-dev meson cmake
```

## Downloading

Release tar files can be downloaded from <https://github.com/5G-MAG/rt-5gc-service-consumers/releases>.

The source can be obtained by cloning the GitHub repository.

For example, to download the latest release you can use:

```bash
cd ~
git clone --recurse-submodules https://github.com/5G-MAG/rt-5gc-service-consumers.git
```

## Building

The build process requires a working Internet connection as project dependencies are downloaded during the build.

To build the libraries and tools from the source:

```bash
cd ~/rt-5gc-service-consumers
meson build
ninja -C build
```

## Installing

To install the built libraries and tools:

```bash
cd ~/rt-5gc-service-consumers/build
sudo meson install --no-rebuild
```

## Running

### PCF PolicyAuthorization tool

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


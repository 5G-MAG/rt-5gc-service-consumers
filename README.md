<h1 align="center">5G Core Service Consumers</h1>
<p align="center">
  <a href="#"><img src="https://img.shields.io/badge/Status-Under_Development-yellow" alt="Under Development"></a>
  <a href="https://github.com/5G-MAG/rt-5gc-service-consumers/releases/latest"><img src="https://img.shields.io/github/v/release/5G-MAG/rt-5gc-service-consumers?label=Version" alt="Version"></a>
  <a href="https://drive.google.com/file/d/1cinCiA778IErENZ3JN52VFW-1ffHpx7Z/view"><img src="https://img.shields.io/badge/License-5G--MAG%20Public%20License%20(v1.0)-blue" alt="License"></a>
</p>

## Introduction

The 5G Core presents several Network Functions, each of which has its own set of service interfaces. This is a
collection of
reusable service consumer libraries designed to talk to the 5G Core Network Functions using some of these service
interfaces.
These interfaces are based upon Open5GS v2.7.2.

In addition, command line tools are provided to demonstrate the use of these service consumer libraries.

Additional information can be found at: https://5g-mag.github.io/Getting-Started/pages/5g-core-network-components/

### About the implementation

These libraries and tools are based upon the [Open5GS](https://open5gs.org/) framework.

## Service consumer libraries

### `libscbsf` - Binding Support Function (BSF) service consumer library

The Binding Support Function (BSF) is responsible for maintaining a mapping between UE PDU Session and the PCF which is
managing that PDU Session.

The `libscbsf` library aids in discovery of the BSF in the 5G Core (by interrogating the NRF) and subsequently looking
up which PCF is managing the PDU Session for a UE, identified by its IP address.

This library implements the service consumer end of the following service-based APIs:

- *Nbsf_Management*

### `libscpcf` - Policy Control Function (PCF) service consumer library

The Policy Control Function (PCF) is responsible for applying charging and network policy to the PDU sessions of UEs.
The *Npcf_PolicyAuthorization* service API is used at reference point N5 by an Application Function (AF) to request
policy changes to the PDU session on behalf of the UE. This allows an Application Function to manipulate particular
network QoS parameters for selected IP traffic flows within the PDU session.

The `libscpcf` library allows an application to connect to a PCF and request an `AppSessionContext` which it can then
use to manipulate the network routing policies for traffic passing across specific application flows within a UE's PDU
Session.

This library implements the service consumer end of the following service-based APIs:

- *Npcf_PolicyAuthorization*

### `libscmbsmf` - Multicast/Broadcast Session Management Function (MB-SMF) service consumer library

The Multicast/Broadcast Session Management Function (MB-SMF) is responsible for allocating and deallocating Temporary
Mobile Group Identities (TMGIs) and for the management of Multicast/Broadcast Services (MBS) on the Multicast/Broadcast
User Plane Function (MB-UPF). The *Nmbsmf_TMGI* service API is used at reference point Nmb1 for the allocation and
deallocation of TMGIs, and the *Nmbsmf_MBSSession* service API is used to reference point Nmb1 for the creation,
modification and destruction of MBS Sessions and for the management of notification subscriptions to events arising on
those MBS Sessions. This provides a Network Function with the ability to setup MBS Sessions for Multicast/Broadcast
distribution to UEs and to remove those MBS Sessions once the Multicast/Broadcast channel is no longer needed.

The `libscmbsmf` library provides a simple create/destroy interface for TMGI management and an MBS Session and
notifications subscriptions model for management of MBS Sessions.

This library implements the service consumer end of the following service-based APIs:

- *Nmbsmf_TMGI*
- *Nmbsmf_MBSSession*

## Command line tools

### `pcf-policyauthorization`

The `pcf-policyauthorization` tool manipulates the network Quality of Service parameters of Application Session Contexts
in the PCF by using the **PCF service consumer library** to invoke operations on the *Npcf_PolicyAuthorization* service
API.

The PCF address can be explicitly specified at the command line if this is already known. Alternatively, the tool can
also use the **BSF service consumer library** to look up which PCF instance is managing the PDU Session of interest (
based on the IP address of a UE registered with the AMF).

### `tmgi-tool`

The `tmgi-tool` provides a simple command line interface to either request the creation or destruction of a TMGI using
the interfaces provided by the **MB-SMF service consumer library** to invoke operations on the *Nmbsmf_TMGI* service
API.

### `mbs-session-tool`

The `mbs-session-tool` can register an MBS Session and will then wait for notifications for that MBS Session. It does
this by using the interfaces provided by the **MB-SMF service consumer library** to invoke operations on the
*Nmbsmf_MBSSession* service API.

## Install dependencies

To build and use the service consumer libraries and accompanying command line tools, you will need to install the
following packages:

```bash
sudo apt install git ninja-build build-essential flex bison libsctp-dev libgnutls28-dev libgcrypt-dev libssl-dev libidn11-dev libmongoc-dev libbson-dev libyaml-dev libnghttp2-dev libmicrohttpd-dev libcurl4-gnutls-dev libnghttp2-dev libtins-dev libtalloc-dev meson cmake libpcre2-dev pkg-config

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

### Building documentation

You can optionally build documentation. For this you will need `doxygen` installed. If you want diagrams in the
documentation you
will also need `dot` and `plantuml`.

To install the documentation dependencies on Ubuntu use the command:

```bash
sudo apt install doxygen graphviz plantuml
```

To build the documentation:

```bash
cd ~/rt-5gc-service-consumers
meson setup --reconfigure build -Dbuild_docs=true
ninja -C build docs
```

The documentation will then be found under the `~/rt-5gc-service-consumers/build/docs` directory.

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

## Acknowledgements

Development of the BSF and PCF service consumer libraries was funded by the UK Government through
the [REASON](https://reason-open-networks.ac.uk/) project.

## Troubleshooting

### Wrong meson version

In case the `meson` version that is installed via `apt` does not fulfill the version requirements you can install
`meson` via `pipx`. As an example, you might get the following error when building the project:

`meson.build:12:20: ERROR: Meson version is 1.3.2 but project requires >= 1.4.0`

In this case remove the previously installed `meson` version and reinstall via `pipx`:

``` 
sudo apt-get remove meson
pipx install meson
pipx ensurepath
```

Restart your shell and then run `meson build` again.


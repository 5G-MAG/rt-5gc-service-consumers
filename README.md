<p align="center">
  <img src=".github/banner.svg" width="100%" alt="Reference Tools · 5G Core Service Consumers: 5G Core Service Consumers">
</p>

<p align="center">
  Reusable service consumer libraries, and command line tools that use them, for talking to the
  BSF, PCF and MB-SMF Network Functions of a 5G Core through their service-based APIs.
</p>

<p align="center">
  <img alt="Status: under development"
    src="https://img.shields.io/badge/Status-Under%20Development-e67e22">
  <a href="https://github.com/5G-MAG/rt-5gc-service-consumers/releases"><img alt="Version"
    src="https://img.shields.io/github/v/release/5G-MAG/rt-5gc-service-consumers?label=Version"></a>
  <a href="LICENSE"><img alt="License: 5G-MAG Public License v1.0"
    src="https://img.shields.io/badge/License-5G--MAG%20PL%20v1.0-blue"></a>
</p>

<p align="center">
  <a href="https://www.5g-mag.com/reference-tools/5g-core/">Project page</a> &nbsp;&middot;&nbsp;
  <a href="https://github.com/5G-MAG/rt-5gc-service-consumers/issues">Issues</a> &nbsp;&middot;&nbsp;
  <a href="https://www.5g-mag.com/contributing">Contributing</a>
</p>

---

## At a glance

|  |  |
|---|---|
| **Part of** | [5G Core Service Consumers](https://www.5g-mag.com/reference-tools/5g-core/) |

## Introduction

Each 5G Core Network Function offers its own set of service interfaces. This repository is a
collection of reusable libraries that act as the service consumer for some of those interfaces, plus
command line tools that show how to use them. The libraries and tools are built on the
[Open5GS](https://open5gs.org/) framework, and the interfaces are based on Open5GS v2.7.2.

More information is on the [project page](https://www.5g-mag.com/reference-tools/5g-core/).

### Service consumer libraries

#### `libscbsf` - Binding Support Function (BSF) service consumer library

The Binding Support Function (BSF) maintains the mapping between a UE's PDU Session and the PCF that
manages that PDU Session.

The `libscbsf` library discovers the BSF in the 5G Core (by querying the NRF) and then looks up which
PCF manages the PDU Session of a UE, identified by its IP address.

It implements the service consumer end of this service-based API:

- *Nbsf_Management*

#### `libscpcf` - Policy Control Function (PCF) service consumer library

The Policy Control Function (PCF) applies charging and network policy to the PDU Sessions of UEs.
An Application Function (AF) uses the *Npcf_PolicyAuthorization* service API at reference point N5
to request policy changes to a PDU Session on behalf of the UE, for example to change network QoS
parameters for selected IP traffic flows within that PDU Session.

The `libscpcf` library lets an application connect to a PCF and request an `AppSessionContext`,
which it can then use to change the network routing policies for traffic on specific application
flows within a UE's PDU Session.

It implements the service consumer end of this service-based API:

- *Npcf_PolicyAuthorization*

#### `libscmbsmf` - Multicast/Broadcast Session Management Function (MB-SMF) service consumer library

The Multicast/Broadcast Session Management Function (MB-SMF) allocates and deallocates Temporary
Mobile Group Identities (TMGIs) and manages Multicast/Broadcast Services (MBS) on the
Multicast/Broadcast User Plane Function (MB-UPF). At reference point Nmb1, the *Nmbsmf_TMGI* service
API allocates and deallocates TMGIs, and the *Nmbsmf_MBSSession* service API creates, modifies and
destroys MBS Sessions and manages subscriptions to notifications of events on them. A Network
Function can therefore set up MBS Sessions for multicast or broadcast distribution to UEs, and
remove them when the channel is no longer needed.

The `libscmbsmf` library provides a simple create/destroy interface for TMGI management, and an MBS
Session and notification subscription model for managing MBS Sessions.

It implements the service consumer end of these service-based APIs:

- *Nmbsmf_TMGI*
- *Nmbsmf_MBSSession*

### Command line tools

#### `pcf-policyauthorization`

The `pcf-policyauthorization` tool changes the network Quality of Service parameters of Application
Session Contexts in the PCF, using the **PCF service consumer library** to invoke operations on the
*Npcf_PolicyAuthorization* service API.

The PCF address can be given on the command line if it is already known. Otherwise the tool can use
the **BSF service consumer library** to look up which PCF instance manages the PDU Session of
interest, based on the IP address of a UE registered with the AMF.

#### `tmgi-tool`

The `tmgi-tool` is a simple command line interface to request the creation or destruction of a TMGI,
using the **MB-SMF service consumer library** to invoke operations on the *Nmbsmf_TMGI* service API.

#### `mbs-service-tool`

The `mbs-service-tool` registers an MBS Session and then waits for notifications for it, using the
**MB-SMF service consumer library** to invoke operations on the *Nmbsmf_MBSSession* service API.

### Acknowledgements

Development of the BSF and PCF service consumer libraries was funded by the UK Government through
the [REASON](https://reason-open-networks.ac.uk/) project.

## Install dependencies

To build and use the libraries and command line tools, install these packages:

```bash
sudo apt install git ninja-build build-essential flex bison libsctp-dev libgnutls28-dev libgcrypt-dev libssl-dev libidn11-dev libmongoc-dev libbson-dev libyaml-dev libnghttp2-dev libmicrohttpd-dev libcurl4-gnutls-dev libnghttp2-dev libtins-dev libtalloc-dev libpcre2-dev meson cmake python3-pip
```

## Downloading

Release tar files can be downloaded from <https://github.com/5G-MAG/rt-5gc-service-consumers/releases>.

The source can also be obtained by cloning the GitHub repository. For example, to download the
latest release:

```bash
cd ~
git clone --recurse-submodules https://github.com/5G-MAG/rt-5gc-service-consumers.git
```

## Building

The build needs a working Internet connection, as project dependencies are downloaded during the
build.

To build the libraries and tools from source:

```bash
cd ~/rt-5gc-service-consumers
meson build
ninja -C build
```

### Building documentation

Building the documentation is optional and needs `doxygen`. For diagrams in the documentation you
also need `dot` and `plantuml`.

To install the documentation dependencies on Ubuntu:

```bash
sudo apt install doxygen graphviz plantuml
```

To build the documentation:

```bash
cd ~/rt-5gc-service-consumers
meson setup --reconfigure build -Dbuild_docs=true
ninja -C build docs
```

The documentation is then in the `~/rt-5gc-service-consumers/build/docs` directory.

## Installing

To install the built libraries and tools:

```bash
cd ~/rt-5gc-service-consumers/build
sudo meson install --no-rebuild
```

## Running

See the [**Tutorials**](https://www.5g-mag.com/reference-tools/5g-core/tutorials/)
for how the different libraries are used in context.

Each tool's command help describes how to use it.

For the **PCF PolicyAuthorization** tool:

```bash
/usr/local/bin/pcf-policyauthorization -h
```

For the **TMGI Allocation and Deallocation** tool:

```bash
/usr/local/bin/tmgi-tool -h
```

For the **MBS Service** tool:

```bash
/usr/local/bin/mbs-service-tool -h
```

### Troubleshooting

#### Wrong meson version

If the `meson` version installed via `apt` does not meet this project's version requirement, the
build fails with an error like this:

`meson.build:12:20: ERROR: Meson version is 1.3.2 but project requires >= 1.4.0`

In that case, upgrade `meson` via `python3`:

``` 
sudo apt-get remove meson
sudo python3 -m pip install --break-system-packages --upgrade meson
```

#### libscbsf.so.2 not found

If the `libscbsf.so.2` library is not found when running one of the tools, find the library with:

```
find /usr/local -name 'libscbsf.so*' 2>/dev/null
```

This should return the location of the library, for example:

``` 
/usr/local/lib/x86_64-linux-gnu/libscbsf.so.1.0.0
/usr/local/lib/x86_64-linux-gnu/libscbsf.so.2.0.0
/usr/local/lib/x86_64-linux-gnu/libscbsf.so
/usr/local/lib/x86_64-linux-gnu/libscbsf.so.2
/usr/local/lib/x86_64-linux-gnu/libscbsf.so.1
```

Then add that path to a configuration file so the dynamic linker can find it:

```
echo '/usr/local/lib/x86_64-linux-gnu' | sudo tee /etc/ld.so.conf.d/usr-local-x86_64.conf
sudo ldconfig
```

#### pcf-policyauthorization: PCF rejecting AppSessionContext

The Open5GS PCF rejects AppSessionContext requests if the Media-Type in the requested QoS is not
`audio`, `video` or `control`. It also rejects them if no default PCC Rule has been configured in the
Open5GS Core for the 5QI associated with the Media-Type. You need a default PCC Rule for 5QI 1 for
the audio Media-Type, 5QI 2 for video and 5QI 5 for control. The default PCC Rules for a subscriber
UE can be configured in the Open5GS WebUI.

## Development

This project follows the
[Gitflow workflow](https://www.atlassian.com/git/tutorials/comparing-workflows/gitflow-workflow). The
`development` branch is the integration branch for new features, so switch to the `development`
branch before starting work on a new feature.

## Contributing

Contributions are welcome. How to raise an issue, fork the repository and open a pull request, and
the Contributor License Agreement required before code can be merged, are described at
<https://www.5g-mag.com/contributing>.

## License

Distributed under the 5G-MAG Public License v1.0. See [LICENSE](LICENSE).

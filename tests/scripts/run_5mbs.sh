#!/bin/bash
#################################################################################
# 5G-MAG Reference Tools: 5GC Service Consumers Open5GS start-up script for 5MBS
#################################################################################
# Copyright: (C) 2025 British Broadcasting Corporation
# Author: David Waring <david.waring2@bbc.co.uk>
# License: 5G-MAG Public License (v1.0)
#
# For full license terms please see the LICENSE file distributed with this
# program. If this file is missing then the license can be retrieved from
# https://github.com/5G-MAG/Getting-Started/blob/gh-pages/OFFICIAL_5G-MAG_Public_License_v1.0.pdf
#################################################################################
#
# This script will start the Open5GS services from the installation location that
# are needed for 5MBS testing.
#
#################################################################################

if [[ "$(id -u)" != 0 ]]; then
    x_flag=""
    if grep -q x <<<"$-"; then
        x_flag="-x"
    fi
    echo "Re-running as root, enter your password for sudo access if prompted" 1>&2
    exec /bin/sudo -E -u '#0' -s /bin/bash $x_flag "$0" --as-user="#$(id -u)" "$@"
fi

script_dir="$(realpath "$(dirname "$0")")"
src_root_dir="$(realpath "${script_dir}/../..")"
prefix_dir="$(cd $src_root_dir; meson introspect --buildoptions build | jq -r '.[] | select(.section == "directory" and .name == "prefix") | .value')"

bindir="$prefix_dir/bin"
libdir="$prefix_dir/lib64"

as_user=""

while [[ $# -gt 0 ]]; do
  arg="$1"
  shift
  case "$arg" in
  --as-user=*)
    as_user="sudo -E -u ${arg#*=}"
    ;;
  *)
    echo "Unknown command line option: $arg" >&2
    exit 1
    ;;
  esac
done

# Start NRF as a user service first
user_services="nrf scp amf smf"
root_services="upf"

export LD_LIBRARY_PATH="$libdir"

if [[ -n "${user_services}" ]]; then
    for nf in ${user_services}; do
        ${as_user} "${bindir}/open5gs-${nf}d" >& "${nf}.log" &
        if [[ "$nf" = "nrf" ]]; then
            sleep 2
        fi
    done
fi

if [[ -n "${root_services}" ]]; then
    for nf in ${root_services}; do
        "$bindir/open5gs-${nf}d" >& "${nf}.log" &
    done
fi

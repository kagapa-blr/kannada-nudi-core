#!/bin/sh
set -eu

repo_root="$(CDPATH= cd -- "$(dirname "$0")/.." && pwd)"

if ! grep -q 'ibus_bus_register_component' "$repo_root/src/platform/linux/ibus_engine.cpp"; then
  echo 'Missing IBus component registration in src/platform/linux/ibus_engine.cpp' >&2
  exit 1
fi

if ! grep -q '<name>nudi</name>' "$repo_root/com.kagapa.Nudi.xml"; then
  echo 'Component XML is missing the nudi engine definition' >&2
  exit 1
fi

if ! grep -q '<exec>/usr/libexec/ibus-engine-nudi</exec>' "$repo_root/com.kagapa.Nudi.xml"; then
  echo 'Component XML is missing the installed ibus engine path' >&2
  exit 1
fi

echo 'IBus registration contract is present.'

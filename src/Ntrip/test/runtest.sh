#!/bin/bash
set -ex

cd "$( dirname $( readlink --canonicalize "${BASH_SOURCE}" ) )"

rm -f -- *.log *.xml

MAKE="make --makefile test-Ntrip.mk"

${MAKE} clean
${MAKE} .pre
${MAKE} check
${MAKE} clean

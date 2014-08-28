#!/usr/bin/sh

if [[ $# -eq 1 && -n "$1" ]] ; then
	IDENTIFIER="$1"
else
	IDENTIFIER=$(date '+%Y%m%d-%H%M%S')
fi
TIMING_FILE="/tmp/script-timing-${IDENTIFIER}.txt"
TYPESCRIPT_FILE="/tmp/script-typescript-${IDENTIFIER}.txt"

echo "identifier=${IDENTIFIER}"
echo "timing file=${TIMING_FILE}"
echo "typescript file=${TYPESCRIPT_FILE}"

echo "---------------------------------------------------"
script "--timing=${TIMING_FILE}" "${TYPESCRIPT_FILE}"
echo "---------------------------------------------------"

ls -l "${TIMING_FILE}" "${TYPESCRIPT_FILE}"

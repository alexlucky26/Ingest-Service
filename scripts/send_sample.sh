#!/usr/bin/env bash
set -euo pipefail
# send sample CSV to ingest
curl -v --header "Content-Type: text/csv" --data-binary @../data/data_sample.csv http://localhost:8080/ingest
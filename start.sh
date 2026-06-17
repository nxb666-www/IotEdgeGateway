#!/bin/bash

./build/bin/iotgw_gateway \
    --yaml-config config/environments/development.yaml \
    --log-level debug

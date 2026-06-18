#!/bin/bash

./build/iotgw_gateway \
    --yaml-config config/environments/development.yaml \
    --log-level debug

#!/bin/bash
source ~/dev/patreos/patreos-contracts/local/constants.sh

./bootstrap_patreos.sh

./simulation/setup.sh

./simulation/patreos_users.sh

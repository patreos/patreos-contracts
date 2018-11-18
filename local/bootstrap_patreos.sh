#!/bin/bash

# Public/Private keypairs
#
# eosio
# Private key: 5KQwrPbwdL6PhXujxW37FSSQZ1JiwsST4cqQzDeyXtP79zkvFD3
# Public key: EOS6MRyAjQq8ud7hVNYcfnVPJqcVpscN5So8BhtHuGYqET5GDW5CV
#
# patreos test contracts
# Private key: 5JoEQenCL5WEaWyxZCRbgGvxuyKnWYUcBKmtD1oc3HnYfBttHPB
# Public key: EOS7YZQ7PbeFY8KPA9XAe3K7dkME3JKwWadMcggRPeVHRQ2DZDeoZ
#
# patreos test users
# Private key: 5J8CZFE5kM8bvaJ9EbotZBvGubkkXtDreLED6g6o5XBJNyY9Ldz
# Public key: EOS6yLaA9fEfP6631e8ofPSfEeomnVwGTZLh9ZpgkogErAewjwzj8
#
# Private key: 5JBM4Dw7TxGEnCTFcTWLn2U7BPX9pfHuBTitNFnKX4gGhtpLATp
# Public key: EOS8gFRod5S9fimS45dQfnsvwzBe2VsJBDTGgrBAG5NWLWKscaL3Z
#
# Private key: 5HztyDL73g4Ctb3CmSTmv698nLPWL8QLUkyJ5WC4Qh6xz4UT2AK
# Public key: EOS6gpKb2WC7K8N33xHxiQn2dkxdrqpF4E6j8upaBsbrXjPfhxDzd
#
# Private key: 5JgxBtd2QwvDV8RkX7ukWrcFfdjK6k9z1bw6fXEYuqhvmAJ4dLm
# Public key: EOS6ipS4diKcXjaNYA3NXApRivnaJ5g86L1LmzmrFAzr4n8J2CJny
#
# Private key: 5KXTErr7PQLEWtYzDPa6LCXxM8S4pYZVNGKnU3TdsfLdvPEQF4z
# Public key: EOS6CvgZn6D5f8fRof8FyogKWdJ5qYxiwFkCQTA5bi9BfGgKThRB4
#
# Private key: 5KgggZQ3vrF7T8kaXgQ3ePvvCLG31iUm2wEAn7r258LiZVCiQjq
# Public key: EOS4wAP4BFUAMLtt3WkXeVssY8jPQrr2tfAfpNuuDU92F4yDCxLed
#
# Private key: 5JXT8YAsNvtipDH6oFon75EdFLHZ1x8VmwgBz2kz3be7Jr3kW4S
# Public key: EOS5zND1SyAhuCWE1zTsdVdZvewME9vojtYXZ4ygKHsX6mv2TksSb

SYSTEM_TOKEN=SYS
PATREOS_TOKEN=PATR

UNSECURE_WALLET_PWD=PW5JUuzAq3pU7u9GGZsTBsT5Tu34Nh9Qmfx4hrxe8Dqk62nhsZyEL

PRIVATE_KEYS=(
  5JXT8YAsNvtipDH6oFon75EdFLHZ1x8VmwgBz2kz3be7Jr3kW4S
  5J8CZFE5kM8bvaJ9EbotZBvGubkkXtDreLED6g6o5XBJNyY9Ldz
  5JBM4Dw7TxGEnCTFcTWLn2U7BPX9pfHuBTitNFnKX4gGhtpLATp
  5HztyDL73g4Ctb3CmSTmv698nLPWL8QLUkyJ5WC4Qh6xz4UT2AK
  5JgxBtd2QwvDV8RkX7ukWrcFfdjK6k9z1bw6fXEYuqhvmAJ4dLm
  5KXTErr7PQLEWtYzDPa6LCXxM8S4pYZVNGKnU3TdsfLdvPEQF4z
  5KgggZQ3vrF7T8kaXgQ3ePvvCLG31iUm2wEAn7r258LiZVCiQjq
)

PUBLIC_KEYS=(
  EOS5zND1SyAhuCWE1zTsdVdZvewME9vojtYXZ4ygKHsX6mv2TksSb
  EOS6yLaA9fEfP6631e8ofPSfEeomnVwGTZLh9ZpgkogErAewjwzj8
  EOS8gFRod5S9fimS45dQfnsvwzBe2VsJBDTGgrBAG5NWLWKscaL3Z
  EOS6gpKb2WC7K8N33xHxiQn2dkxdrqpF4E6j8upaBsbrXjPfhxDzd
  EOS6ipS4diKcXjaNYA3NXApRivnaJ5g86L1LmzmrFAzr4n8J2CJny
  EOS6CvgZn6D5f8fRof8FyogKWdJ5qYxiwFkCQTA5bi9BfGgKThRB4
  EOS4wAP4BFUAMLtt3WkXeVssY8jPQrr2tfAfpNuuDU92F4yDCxLed
)

echo "Unlocking Unsecure Wallet..."
cleos wallet lock -n unsecure
cleos wallet unlock -n unsecure --password $UNSECURE_WALLET_PWD

echo "Adding EOSIO private key to unsecure wallet"
cleos wallet import -n unsecure --private-key 5KQwrPbwdL6PhXujxW37FSSQZ1JiwsST4cqQzDeyXtP79zkvFD3

echo "Adding PATREOS private key to unsecure wallet"
cleos wallet import -n unsecure --private-key 5JoEQenCL5WEaWyxZCRbgGvxuyKnWYUcBKmtD1oc3HnYfBttHPB

for key in "${PRIVATE_KEYS[@]}"
do
  echo "Adding key: ${key} to unsecure wallet"
  cleos wallet import -n unsecure --private-key ${key}
done

echo "Setting BIOS Contract..."
cleos set contract eosio ~/dev/eos/eos/build/contracts/eosio.bios

echo "Creating System Accounts..."
sleep 2
cleos create account eosio eosio.bpay EOS6MRyAjQq8ud7hVNYcfnVPJqcVpscN5So8BhtHuGYqET5GDW5CV EOS6MRyAjQq8ud7hVNYcfnVPJqcVpscN5So8BhtHuGYqET5GDW5CV
cleos create account eosio eosio.msig EOS6MRyAjQq8ud7hVNYcfnVPJqcVpscN5So8BhtHuGYqET5GDW5CV EOS6MRyAjQq8ud7hVNYcfnVPJqcVpscN5So8BhtHuGYqET5GDW5CV
cleos create account eosio eosio.names EOS6MRyAjQq8ud7hVNYcfnVPJqcVpscN5So8BhtHuGYqET5GDW5CV EOS6MRyAjQq8ud7hVNYcfnVPJqcVpscN5So8BhtHuGYqET5GDW5CV
cleos create account eosio eosio.ram EOS6MRyAjQq8ud7hVNYcfnVPJqcVpscN5So8BhtHuGYqET5GDW5CV EOS6MRyAjQq8ud7hVNYcfnVPJqcVpscN5So8BhtHuGYqET5GDW5CV
cleos create account eosio eosio.ramfee EOS6MRyAjQq8ud7hVNYcfnVPJqcVpscN5So8BhtHuGYqET5GDW5CV EOS6MRyAjQq8ud7hVNYcfnVPJqcVpscN5So8BhtHuGYqET5GDW5CV
cleos create account eosio eosio.saving EOS6MRyAjQq8ud7hVNYcfnVPJqcVpscN5So8BhtHuGYqET5GDW5CV EOS6MRyAjQq8ud7hVNYcfnVPJqcVpscN5So8BhtHuGYqET5GDW5CV
cleos create account eosio eosio.stake EOS6MRyAjQq8ud7hVNYcfnVPJqcVpscN5So8BhtHuGYqET5GDW5CV EOS6MRyAjQq8ud7hVNYcfnVPJqcVpscN5So8BhtHuGYqET5GDW5CV
cleos create account eosio eosio.token EOS6MRyAjQq8ud7hVNYcfnVPJqcVpscN5So8BhtHuGYqET5GDW5CV EOS6MRyAjQq8ud7hVNYcfnVPJqcVpscN5So8BhtHuGYqET5GDW5CV
cleos create account eosio eosio.upay EOS6MRyAjQq8ud7hVNYcfnVPJqcVpscN5So8BhtHuGYqET5GDW5CV EOS6MRyAjQq8ud7hVNYcfnVPJqcVpscN5So8BhtHuGYqET5GDW5CV

echo "Setting eosio.token and eosio.msig Contracts..."
sleep 2
cleos set contract eosio.token ~/dev/eos/eos/build/contracts/eosio.token
cleos set contract eosio.msig ~/dev/eos/eos/build/contracts/eosio.msig

echo "Creating System Token..."
sleep 2
JSON=$(jq -n --arg maximum_supply "1000000000.0000 $SYSTEM_TOKEN" '{ issuer: "eosio", maximum_supply: $maximum_supply, can_freeze: 0, can_recall: 0, can_whitelist: 0 }')
cleos push action eosio.token create "${JSON}" -p eosio.token

echo "Issuing System Tokens to eosio..."
sleep 2
JSON=$(jq -n --arg quantity "1000000000.0000 $SYSTEM_TOKEN" '{ to: "eosio", quantity: $quantity, memo: "issue" }')
cleos push action eosio.token issue "${JSON}" -p eosio

echo "Setting eosio.system Contract..."
cleos set contract eosio ~/dev/eos/eos/build/contracts/eosio.system


PATREOS_USERS=(
  okayplanet12
  inflplanet13
  dropplanet14
  okayplanet15
  okayplanet22
  okayplanet23
  okayplanet24
)

echo "Making user accounts..."
for account in "${PATREOS_USERS[@]}"
do
  echo "Creating Account: ${account} with Public Key: ${PUBLIC_KEYS[0]}"
  cleos system newaccount  --stake-net "100.0000 $SYSTEM_TOKEN" --stake-cpu "100.0000 $SYSTEM_TOKEN" --buy-ram-kbytes 1000 eosio ${account} ${PUBLIC_KEYS[0]} ${PUBLIC_KEYS[0]}
  JSON=$(jq -n --arg to "${account}" --arg quantity "1000.0000 $SYSTEM_TOKEN" '{ from: "eosio", to: $to, quantity: $quantity, memo: "<3" }')
  cleos push action eosio.token transfer "${JSON}" -p eosio
  PUBLIC_KEYS=(${PUBLIC_KEYS[@]:1})
done

PATREOS_CONTRACTS=(
  patreostoken
  patreosnexus
  patreosblurb
  patreosvault
)

echo "Making Patreos Contract Accounts..."
sleep 2
for account in "${PATREOS_CONTRACTS[@]}"
do
  echo "Creating Patreos Contract Account: ${account} with Public Key: EOS7YZQ7PbeFY8KPA9XAe3K7dkME3JKwWadMcggRPeVHRQ2DZDeoZ"
  cleos system newaccount  --stake-net "100.0000 $SYSTEM_TOKEN" --stake-cpu "100.0000 $SYSTEM_TOKEN" --buy-ram-kbytes 1000 eosio ${account} EOS7YZQ7PbeFY8KPA9XAe3K7dkME3JKwWadMcggRPeVHRQ2DZDeoZ EOS7YZQ7PbeFY8KPA9XAe3K7dkME3JKwWadMcggRPeVHRQ2DZDeoZ
done

echo "Transfering System Tokens to Patreos Contract Accounts..."
sleep 2

for account in "${PATREOS_CONTRACTS[@]}"
do
  echo "Transfering $SYSTEM_TOKEN to account: ${account}"
  JSON=$(jq -n --arg to "${account}" --arg quantity "1000.0000 $SYSTEM_TOKEN" '{ from: "eosio", to: $to, quantity: $quantity, memo: "<33" }')
  cleos push action eosio.token transfer "${JSON}" -p eosio
done

echo "Setting Patreos Contracts..."
sleep 2
for contract in "${PATREOS_CONTRACTS[@]}"
do
  echo "Setting Patreos contract: ${contract}"
  cleos set contract $contract ~/dev/patreos/patreos-contracts/cdt/$contract -p $contract
done

echo "Creating Patreos Token with Issuer: ${PATREOS_USERS[0]}"
sleep 2
JSON=$(jq -n --arg issuer "${PATREOS_USERS[0]}" --arg maximum_supply "2000000000.0000 $PATREOS_TOKEN" '{ issuer: $issuer, maximum_supply: $maximum_supply, can_freeze: 0, can_recall: 0, can_whitelist: 0 }')
cleos push action patreostoken create "${JSON}" -p patreostoken

echo "Issuing first Patreos Tokens to ${PATREOS_USERS[0]}"
sleep 2

ISSUING=(
  "$(jq -n --arg to "${PATREOS_USERS[0]}" --arg quantity "680000000.0000 $PATREOS_TOKEN" --arg memo "34% ELSEWHERE" '{ to: $to, quantity: $quantity, memo: $memo }')"
  "$(jq -n --arg to "${PATREOS_USERS[1]}" --arg quantity "120000000.0000 $PATREOS_TOKEN" --arg memo "6% INFLATION" '{ to: $to, quantity: $quantity, memo: $memo }')"
  "$(jq -n --arg to "${PATREOS_USERS[2]}" --arg quantity "1200000000.0000 $PATREOS_TOKEN" --arg memo "60% AIRDROP" '{ to: $to, quantity: $quantity, memo: $memo }')"
)
for issue in "${ISSUING[@]}"
do
  echo "Issuing Patreos Tokens: ${issue}"
  cleos push action patreostoken issue "${issue}" -p ${PATREOS_USERS[0]}
done

echo "Final Stats..."
sleep 2
echo "EOSIO Balance"
cleos get currency balance eosio.token eosio

echo "Get stats for $SYSTEM_TOKEN"
cleos get currency stats eosio.token "$SYSTEM_TOKEN"

echo "Get stats for $PATREOS_TOKEN"
cleos get currency stats patreostoken "$PATREOS_TOKEN"

echo "${PATREOS_USERS[0]} Balance:"
cleos get currency balance patreostoken ${PATREOS_USERS[0]}

echo "${PATREOS_USERS[1]} Balance:"
cleos get currency balance patreostoken ${PATREOS_USERS[1]}

echo "${PATREOS_USERS[2]} Balance:"
cleos get currency balance patreostoken ${PATREOS_USERS[2]}

echo "Finished..."

#!/bin/bash

SYSTEM_TOKEN=SYS
EOS_TOKEN=EOS
PATREOS_TOKEN=PATR

UNSECURE_WALLET_PWD=PW5JUuzAq3pU7u9GGZsTBsT5Tu34Nh9Qmfx4hrxe8Dqk62nhsZyEL

SYSTEM_PUBLIC_KEY=EOS6MRyAjQq8ud7hVNYcfnVPJqcVpscN5So8BhtHuGYqET5GDW5CV
SYSTEM_PRIVATE_KEY=5KQwrPbwdL6PhXujxW37FSSQZ1JiwsST4cqQzDeyXtP79zkvFD3

PATREOS_PUBLIC_KEY=EOS7YZQ7PbeFY8KPA9XAe3K7dkME3JKwWadMcggRPeVHRQ2DZDeoZ
PATREOS_PRIVATE_KEY=5JoEQenCL5WEaWyxZCRbgGvxuyKnWYUcBKmtD1oc3HnYfBttHPB

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
cleos wallet import -n unsecure --private-key $SYSTEM_PRIVATE_KEY

echo "Adding PATREOS private key to unsecure wallet"
cleos wallet import -n unsecure --private-key $PATREOS_PRIVATE_KEY

for key in "${PRIVATE_KEYS[@]}"
do
  echo "Adding key: ${key} to unsecure wallet"
  cleos wallet import -n unsecure --private-key ${key}
done

echo "Setting BIOS Contract..."
cleos set contract eosio ~/dev/eos/eos/build/contracts/eosio.bios

echo "Creating System Accounts..."
sleep 2
cleos create account eosio eosio.bpay $SYSTEM_PUBLIC_KEY $SYSTEM_PUBLIC_KEY
cleos create account eosio eosio.msig $SYSTEM_PUBLIC_KEY $SYSTEM_PUBLIC_KEY
cleos create account eosio eosio.names $SYSTEM_PUBLIC_KEY $SYSTEM_PUBLIC_KEY
cleos create account eosio eosio.ram $SYSTEM_PUBLIC_KEY $SYSTEM_PUBLIC_KEY
cleos create account eosio eosio.ramfee $SYSTEM_PUBLIC_KEY $SYSTEM_PUBLIC_KEY
cleos create account eosio eosio.saving $SYSTEM_PUBLIC_KEY $SYSTEM_PUBLIC_KEY
cleos create account eosio eosio.stake $SYSTEM_PUBLIC_KEY $SYSTEM_PUBLIC_KEY
cleos create account eosio eosio.token $SYSTEM_PUBLIC_KEY $SYSTEM_PUBLIC_KEY
cleos create account eosio eosio.upay $SYSTEM_PUBLIC_KEY $SYSTEM_PUBLIC_KEY

echo "Setting eosio.token and eosio.msig Contracts..."
sleep 2
cleos set contract eosio.token ~/dev/eos/eos/build/contracts/eosio.token
cleos set contract eosio.msig ~/dev/eos/eos/build/contracts/eosio.msig

echo "Creating System Token $SYSTEM_TOKEN..."
sleep 2
JSON=$(jq -n --arg maximum_supply "1000000000.0000 $SYSTEM_TOKEN" '{ issuer: "eosio", maximum_supply: $maximum_supply, can_freeze: 0, can_recall: 0, can_whitelist: 0 }')
cleos push action eosio.token create "${JSON}" -p eosio.token

echo "Creating $EOS_TOKEN Token..."
sleep 2
JSON=$(jq -n --arg maximum_supply "1000000000.0000 $EOS_TOKEN" '{ issuer: "eosio", maximum_supply: $maximum_supply, can_freeze: 0, can_recall: 0, can_whitelist: 0 }')
cleos push action eosio.token create "${JSON}" -p eosio.token

echo "Issuing $SYSTEM_TOKEN to eosio..."
sleep 2
JSON=$(jq -n --arg quantity "1000000000.0000 $SYSTEM_TOKEN" '{ to: "eosio", quantity: $quantity, memo: "issue" }')
cleos push action eosio.token issue "${JSON}" -p eosio

echo "Issuing $EOS_TOKEN to eosio..."
sleep 2
JSON=$(jq -n --arg quantity "1000000000.0000 $EOS_TOKEN" '{ to: "eosio", quantity: $quantity, memo: "issue" }')
cleos push action eosio.token issue "${JSON}" -p eosio

echo "Setting eosio.system Contract..."
cleos set contract eosio ~/dev/eos/eos/build/contracts/eosio.system


PATREOS_USERS=(
  xokayplanetx
  patrinflatex
  patrairdropx
  okayplanet1x
  okayplanet2x
  okayplanet3x
  okayplanet4x
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
  cleos system newaccount  --stake-net "100.0000 $SYSTEM_TOKEN" --stake-cpu "100.0000 $SYSTEM_TOKEN" --buy-ram-kbytes 1000 eosio ${account} $PATREOS_PUBLIC_KEY $PATREOS_PUBLIC_KEY
done

echo "Transfering $SYSTEM_TOKEN to Patreos Contract Accounts..."
sleep 2
for account in "${PATREOS_CONTRACTS[@]}"
do
  echo "Transfering $SYSTEM_TOKEN to account: ${account}"
  JSON=$(jq -n --arg to "${account}" --arg quantity "1000.0000 $SYSTEM_TOKEN" '{ from: "eosio", to: $to, quantity: $quantity, memo: "<33" }')
  cleos push action eosio.token transfer "${JSON}" -p eosio
done

echo "Transfering $EOS_TOKEN to Patreos Contract Accounts..."
sleep 2
for account in "${PATREOS_CONTRACTS[@]}"
do
  echo "Transfering $EOS_TOKEN to account: ${account}"
  JSON=$(jq -n --arg to "${account}" --arg quantity "1000.0000 $EOS_TOKEN" '{ from: "eosio", to: $to, quantity: $quantity, memo: "<33" }')
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

echo ""
echo "----------------"
echo "Final Stats..."
sleep 2
echo "EOSIO Balance"
cleos get currency balance eosio.token eosio

echo "Get stats for $SYSTEM_TOKEN"
cleos get currency stats eosio.token "$SYSTEM_TOKEN"

echo "Get stats for $EOS_TOKEN"
cleos get currency stats eosio.token "$EOS_TOKEN"

echo "Get stats for $PATREOS_TOKEN"
cleos get currency stats patreostoken "$PATREOS_TOKEN"

echo "${PATREOS_USERS[0]} Balance:"
cleos get currency balance patreostoken ${PATREOS_USERS[0]}

echo "${PATREOS_USERS[1]} Balance:"
cleos get currency balance patreostoken ${PATREOS_USERS[1]}

echo "${PATREOS_USERS[2]} Balance:"
cleos get currency balance patreostoken ${PATREOS_USERS[2]}

echo "Finished..."

#!/bin/bash
source ./patreos_constants.sh

echo "Unlocking Unsecure Wallet..."
cleos wallet lock -n unsecure
cleos wallet unlock -n unsecure --password $UNSECURE_WALLET_PWD

echo "Making user accounts..."
for account in "${PATREOS_USERS[@]}"
do
  echo "Creating Profile with user: ${account}"
done

echo "Blurb from ${PATREOS_USERS[3]} to ${PATREOS_USERS[4]}..."
sleep 2
JSON=$(jq -n --arg from "${PATREOS_USERS[3]}" --arg to "${PATREOS_USERS[4]}" '{ from: $from, to: $to, memo: "Blurb!!!!" }')
cleos push action patreosblurb blurb "${JSON}" -p ${PATREOS_USERS[3]}

echo "Blurb from ${PATREOS_USERS[3]} to ${PATREOS_USERS[4]}..."
sleep 2
JSON=$(jq -n --arg from "${PATREOS_USERS[3]}" --arg to "${PATREOS_USERS[4]}" '{ from: $from, to: $to, memo: "Blurb!!!!" }')
cleos push action patreosblurb blurb "${JSON}" -p ${PATREOS_USERS[3]}

echo "${PATREOS_USERS[3]} depositing into patreosvault..."
sleep 2
JSON=$(jq -n \
--arg from "${PATREOS_USERS[3]}" \
--arg quantity "1.0000 $EOS_TOKEN" '{ from: $from, to: "patreosvault", quantity: $quantity, memo: "Deposit in patreosvault" }')
cleos push action eosio.token transfer "${JSON}" -p ${PATREOS_USERS[3]}

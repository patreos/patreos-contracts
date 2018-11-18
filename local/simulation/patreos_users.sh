#!/bin/bash
source ~/dev/patreos/patreos-contracts/local/constants.sh

echo "Unlocking Unsecure Wallet..."
cleos wallet lock -n unsecure
cleos wallet unlock -n unsecure --password $UNSECURE_WALLET_PWD

echo "Making user accounts..."
for account in "${PATREOS_USERS[@]}"
do
  echo "Creating Profile with user: ${account}"
done

echo "Blurb from ${PATREOS_USERS[3]} to ${PATREOS_USERS[4]}..."
JSON=$(jq -n --arg from "${PATREOS_USERS[3]}" --arg to "${PATREOS_USERS[4]}" '{ from: $from, to: $to, memo: "Blurb!!!!" }')
cleos push action patreosblurb blurb "${JSON}" -p ${PATREOS_USERS[3]}

echo "${PATREOS_USERS[3]} depositing $EOS_TOKEN into patreosvault..."
JSON=$(jq -n \
--arg from "${PATREOS_USERS[3]}" \
--arg quantity "10.0000 $EOS_TOKEN" '{ from: $from, to: "patreosvault", quantity: $quantity, memo: "EOS deposit in patreosvault" }')
cleos push action eosio.token transfer "${JSON}" -p ${PATREOS_USERS[3]}

echo "${PATREOS_USERS[3]} depositing $PATREOS_TOKEN into patreosvault..."
JSON=$(jq -n \
--arg from "${PATREOS_USERS[3]}" \
--arg quantity "100.0000 $PATREOS_TOKEN" '{ from: $from, to: "patreosvault", quantity: $quantity, memo: "PATR deposit in patreosvault" }')
cleos push action patreostoken transfer "${JSON}" -p ${PATREOS_USERS[3]}

echo "${PATREOS_USERS[4]} depositing $PATREOS_TOKEN into patreosvault..."
JSON=$(jq -n \
--arg from "${PATREOS_USERS[4]}" \
--arg quantity "150.0000 $PATREOS_TOKEN" '{ from: $from, to: "patreosvault", quantity: $quantity, memo: "PATR deposit in patreosvault" }')
cleos push action patreostoken transfer "${JSON}" -p ${PATREOS_USERS[4]}

echo "What to do with: $ret"

sleep 2

echo "------------------"
echo "Finished"

echo "patreosvault balance for ${PATREOS_USERS[3]}"
cleos get currency balance patreosvault ${PATREOS_USERS[3]}

echo "patreosvault balance for ${PATREOS_USERS[4]}"
cleos get currency balance patreosvault ${PATREOS_USERS[4]}

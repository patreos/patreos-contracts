mkdir -p /tmp/eosio/config
mkdir -p /tmp/eosio/data
cp ./config.ini /tmp/eosio/config/
nodeos -e -p eosio --plugin eosio::chain_api_plugin --plugin eosio::history_api_plugin --config-dir /tmp/eosio/config --data-dir /tmp/eosio/data

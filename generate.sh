contract=$1
pushd ${contract}
eosio-cpp -abigen "${contract}.cpp" -o "${contract}.wasm" --contract "${contract}"
popd

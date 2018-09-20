contract=$1
pushd ${contract}
eosiocpp -o ${contract}.wast ${contract}.cpp
#eosiocpp -g ${contract}.abi ${contract}.cpp
popd

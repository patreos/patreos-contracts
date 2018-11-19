#include <eosiolib/asset.hpp>
#include <eosiolib/eosio.hpp>

#include <string>

namespace eosiosystem {
   class system_contract;
}

using std::string;
using namespace eosio;

class [[eosio::contract("patreosblurb")]] patreosblurb : public contract {

  public:
    using contract::contract;

    [[eosio::action]]
    void blurb( name from, name to, string memo ) {
      eosio_assert( from != to, "cannot blurb to self" );
      require_auth( from );
      eosio_assert( is_account( to ), "to account does not exist");
      require_recipient( to );
    }
};

EOSIO_DISPATCH( patreosblurb, (blurb) )

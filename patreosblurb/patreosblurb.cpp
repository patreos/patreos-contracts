#include <eosiolib/asset.hpp>
#include <eosiolib/eosio.hpp>
#include "../common/messages.hpp"
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
      eosio_assert( from != to, Messages::CANNOT_BLURB_TO_SELF );
      require_auth( from );
      eosio_assert( is_account( to ), Messages::TO_ACCCOUNT_DNE );
      require_recipient( to );
    }
};

EOSIO_DISPATCH( patreosblurb, (blurb) )

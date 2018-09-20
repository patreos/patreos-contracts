#include <eosiolib/eosio.hpp>
#include <eosiolib/print.hpp>
#include <eosiolib/transaction.hpp>
using namespace eosio;

// Needs permission set to eosio.code
class patreospayer : public eosio::contract {
  public:
      using contract::contract;

/*
      /// @abi action
      void pledge( account_name from,
                            account_name to,
                            asset        quantity,
                            string       memo ) {

        transaction out{};
        out.actions.emplace_back(permission_level{_self, N(active)}, N(pet), N(feedpet), std::make_tuple(pet.id));
        out.delay_sec = 0;
        out.send(pet.id, _self);


        action( permission_level{ _self, N(active) },
               N(patreostoken), N(pledge),
               std::make_tuple( from, to, quantity, std::string("") )
        ).send();


      }
*/

      /// @abi action
      void send() {
        action( permission_level{ _self, N(eosio.code) },
               N(patreospayer), N(get),
               std::make_tuple( _self )
        ).send();
      }

      /// @abi action
      void get( account_name user ) {
         require_auth2( _self, N(eosio.code) );
         require_auth( _self );
         print( "Hello, ", name{user} );
      }
};

EOSIO_ABI( patreospayer, (send)(get) )

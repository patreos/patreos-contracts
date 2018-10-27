#include <eosiolib/eosio.hpp>
#include <eosiolib/print.hpp>
#include <eosiolib/asset.hpp>
#include <eosiolib/transaction.hpp>
using namespace eosio;
using std::string;

// Needs permission set to eosio.code
class patreosvault : public eosio::contract {
private:
    /// @abi table pledge i64
    struct pledge {
        uint64_t key; // available_primary_key();
        //account_name from;
        account_name to;
        asset value;
        uint16_t days;
        time last_pledge; // now()

        uint64_t primary_key() const { return to; }

        EOSLIB_SERIALIZE(pledge, (to)(value)(days)(last_pledge))
    };

    typedef multi_index<N(pledges), pledge> pledges;

public:
    using contract::contract;

    void add_balance( account_name owner, asset value, account_name ram_payer )
    {
       accounts to_acnts( _self, owner );
       auto to = to_acnts.find( value.symbol.name() );
       if( to == to_acnts.end() ) {
          to_acnts.emplace( ram_payer, [&]( auto& a ){
            a.balance = value;
          });
       } else {
          to_acnts.modify( to, 0, [&]( auto& a ) {
            a.balance += value;
          });
       }
    }

    void transferAction(uint64_t self, uint64_t code) {
        auto data = unpack_action_data<currency::transfer>();
        if(data.from == self || data.to != self)
            return;

        bool deposit = false;
        if(code == N(eosio.token) && data.quantity.symbol == string_to_symbol(4, "EOS")) {
           deposit = true;
        }
        if(code == N(patreostoken) && data.quantity.symbol == string_to_symbol(4, "PTR")) {
          deposit = true;
        }
        eosio_assert(deposit == true, "We currently do not support this token");
        eosio_assert(data.quantity.is_valid(), "Are you trying to corrupt me?");
        eosio_assert(data.quantity.amount > 0, "When pigs fly");

        add_balance(data.from, data.quantity, self)
    }
};

EOSIO_ABI(patreospayer, (process))

extern "C" void apply(uint64_t receiver, uint64_t code, uint64_t action) {
    switch(action) {
        case N(transfer): return transferAction(receiver, code);
        case N(withdraw): return withdrawAction(receiver, code);
    }
}

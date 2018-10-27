#include <eosiolib/eosio.hpp>
#include <eosiolib/asset.hpp>
#include <eosiolib/action.hpp>
#include <eosiolib/currency.hpp>
#include <eosiolib/multi_index.hpp>

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

    typedef eosio::multi_index<N(pledges), pledge> pledges;

    struct account {
       asset    balance;

       uint64_t primary_key()const { return balance.symbol.name(); }
    };

    typedef eosio::multi_index<N(accounts), account> accounts;

    struct withdraw_data {
        uint64_t user;
    };

    struct process_data {
        uint64_t from;
        uint64_t to;
        asset quantity;
    };

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

    void sub_balance( account_name owner, asset value ) {
       accounts from_acnts( _self, owner );

       const auto& from = from_acnts.get( value.symbol.name(), "no balance object found" );
       eosio_assert( from.balance.amount >= value.amount, "overdrawn balance" );


       if( from.balance.amount == value.amount ) {
          from_acnts.erase( from );
       } else {
          from_acnts.modify( from, owner, [&]( auto& a ) {
              a.balance -= value;
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
           eosio_assert(data.quantity.amount >= 0.1, "Minimum deposit of 0.1 EOS required");
        }
        if(code == N(patreostoken) && data.quantity.symbol == string_to_symbol(4, "PTR")) {
          deposit = true;
          eosio_assert(data.quantity.amount >= 50, "Minimum deposit of 50 PTR required");
        }
        eosio_assert(deposit == true, "We currently do not support this token");
        eosio_assert(data.quantity.is_valid(), "Invalid quantity");
        eosio_assert(data.quantity.amount > 0, "Cannot transfer non-positive amount");

        add_balance(data.from, data.quantity, _self);
    }

    void withdrawAction( uint64_t self, uint64_t code ) {

    }

    void process( uint64_t self, uint64_t code )
    {
      auto data = unpack_action_data<process_data>();

        account_name from = data.from;
        account_name to = data.to;
        asset quantity = data.quantity;

        //patreosnexus
        auto plegestable = pledges(N(patreosnexus), from);
        auto existing = plegestable.find(to);
        eosio_assert( existing != plegestable.end(), "Pledge does not exist." );

        // TODO: Check that pledge quantity valid and set var if removed pledge

        // Verify subscription date
        double milliseconds = double(now() - existing->last_pledge);
        int days = (int) ( milliseconds / (1000 * 60 * 60 * 24) );
        if(days >= existing->days) {
          // Pay pledge amount
          add_balance(to, quantity, _self);
          sub_balance(from, quantity);

          // Increment execution_count and update last_pledge
          action(permission_level{ _self, N(eosio.code) },
              N(patreosnexus), N(pledge_paid),
              std::make_tuple(from, to)).send();
        }
    }
};

//EOSIO_ABI(patreosvault, (transferAction)(withdrawAction))

extern "C" void apply(uint64_t receiver, uint64_t code, uint64_t action) {
    auto self = receiver;
    patreosvault thiscontract( self );
    switch(action) {
        case N(transfer): return thiscontract.transferAction(receiver, code);
        case N(withdraw): return thiscontract.withdrawAction(receiver, code);
        case N(process): return thiscontract.process(receiver, code);
    }
}

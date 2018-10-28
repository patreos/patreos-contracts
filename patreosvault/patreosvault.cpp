#include <eosiolib/eosio.hpp>
#include <eosiolib/asset.hpp>
#include <eosiolib/action.hpp>
#include <eosiolib/currency.hpp>
#include <eosiolib/multi_index.hpp>
#include <cmath>

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

    const double eos_fee_double = 0.1;
    const double patreos_fee_double = 50;

    const uint64_t EOS_PRECISION = 4;
    const symbol_type EOS_SYMBOL = S(EOS_PRECISION, EOS);
    const symbol_type PTR_SYMBOL = S(EOS_PRECISION, PTR);

    const asset patreos_fee = eosio::asset((uint64_t)(std::pow(10, EOS_PRECISION) * patreos_fee_double), PTR_SYMBOL);
    const asset eos_fee = eosio::asset((uint64_t)(std::pow(10, EOS_PRECISION) * eos_fee_double), EOS_SYMBOL);

    const uint64_t EOS_TOKEN_CODE = N(eosio.token);
    const uint64_t PATREOS_TOKEN_CODE = N(patreostoken);
    const uint64_t PATREOS_NEXUS_CODE = N(patreosnexus);


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

        bool valid_deposit = false;
        if(code == EOS_TOKEN_CODE && data.quantity.symbol == EOS_SYMBOL) {
           valid_deposit = true;
           eosio_assert(data.quantity.amount >= eos_fee.amount, "Minimum deposit of 0.1 EOS required");
        }
        if(code == PATREOS_TOKEN_CODE && data.quantity.symbol == PTR_SYMBOL) {
          valid_deposit = true;
          eosio_assert(data.quantity.amount >= patreos_fee.amount, "Minimum deposit of 50 PTR required");
        }
        eosio_assert(valid_deposit, "We currently do not support this token");
        eosio_assert(data.quantity.is_valid(), "Invalid quantity");
        eosio_assert(data.quantity.amount > 0, "Cannot transfer non-positive amount");

        add_balance(data.from, data.quantity, _self);
    }

    void withdraw( asset quantity ) {

    }

    void process( uint64_t self, uint64_t code )
    {
      auto data = unpack_action_data<process_data>();

        account_name from = data.from;
        account_name to = data.to;
        asset quantity = data.quantity;

        //patreosnexus
        auto plegestable = pledges(PATREOS_NEXUS_CODE, from);
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
              PATREOS_NEXUS_CODE, N(pledge_paid),
              std::make_tuple(from, to)).send();
        }
    }
};

extern "C" void apply(uint64_t receiver, uint64_t code, uint64_t action) {
    auto self = receiver;
    patreosvault thiscontract( self );
    if (code == self) { // Calling our contract actions
      switch (action) {
        EOSIO_API(patreosvault, (process)(withdraw))
      }
    } else if(action == N(transfer)) { // Another contract making a transfer to contract account
      return thiscontract.transferAction(receiver, code);
      /*
        // another option is to check code here, and pass unpacked data
        eosio::execute_action( &thiscontract, &patreosvault::transferAction );
      */
    } else {
      eosio_exit(0);
    }
}

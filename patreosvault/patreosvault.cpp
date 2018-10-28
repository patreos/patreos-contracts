#include <eosiolib/eosio.hpp>
#include <eosiolib/asset.hpp>
#include <eosiolib/action.hpp>
#include <eosiolib/currency.hpp>
#include <eosiolib/multi_index.hpp>
#include <../patreos/commons.hpp>

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
        uint32_t seconds;
        time last_pledge; // now()

        uint64_t primary_key() const { return to; }

        EOSLIB_SERIALIZE(pledge, (to)(value)(seconds)(last_pledge))
    };

    typedef eosio::multi_index<N(pledges), pledge> pledges;

    struct account {
       asset    balance;
       uint64_t primary_key()const { return balance.symbol.name(); }
    };

    typedef eosio::multi_index<N(accounts), account> accounts;


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

    void withdraw( uint64_t account, asset quantity ) {
      require_auth( account );
    }

    // Process outstanding subscriptions
    void process( uint64_t processor, uint64_t from, uint64_t to, asset quantity )
    {
        eosio_assert(has_auth(to) || has_auth(PATREOS_NEXUS_CODE),
          "Not authorized to process this subscription");
        eosio_assert( is_supported_asset(quantity), "We do not support this token currently");

        // Find pledge in patreosnexus
        auto plegestable = pledges(PATREOS_NEXUS_CODE, from);
        auto existing = plegestable.find(to);
        eosio_assert( existing != plegestable.end(), "Pledge does not exist." );

        // Check that action asset is valid and consistent with pledge asset
        eosio_assert( quantity.is_valid(), "invalid quantity" );
        eosio_assert( quantity.amount > 0, "must transfer positive quantity" );
        eosio_assert( quantity.symbol == existing->value.symbol, "symbol precision mismatch" );

        // Check from account has the funds
        accounts from_acnts( _self, from );
        const auto& match = from_acnts.get( quantity.symbol.name(), "no balance object found" );
        if(match.balance.amount < quantity.amount) {
          // Cancel pledge in patreosnexus due to insufficent funds, someone is naughty
          action(permission_level{ _self, N(eosio.code) },
              PATREOS_NEXUS_CODE, N(depledge),
              std::make_tuple(from, to)).send();
        } else {
          // Verify subscription due date
          double milliseconds_since_last_pledge = double(now() - existing->last_pledge);
          int seconds_since_last_pledge = (int) ( milliseconds_since_last_pledge / 1000 );
          eosio_assert( existing->seconds <= seconds_since_last_pledge, "Pledge subscription not due" );

          require_recipient( from );
          require_recipient( to );

          asset fee;
          if(quantity.symbol == EOS_SYMBOL) {
            fee = eos_fee;
          } else if (quantity.symbol == PTR_SYMBOL) {
            fee = patreos_fee;
          } else {
            eosio_assert( false, "Token fee could not be found" );
          }

          // execute subscription
          sub_balance(from, quantity);
          add_balance(to, quantity - fee, _self);
          add_balance(processor, fee, _self); // fee to processor

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

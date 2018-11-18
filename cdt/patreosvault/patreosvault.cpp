#include <eosiolib/eosio.hpp>
#include <eosiolib/asset.hpp>
#include <eosiolib/action.hpp>
#include <eosiolib/multi_index.hpp>
#include "patreosvault.hpp"

using namespace eosio;

void patreosvault::sub_balance( name owner, asset value ) {
   accounts from_acnts( _self, owner.value );

   const auto& from = from_acnts.get( value.symbol.code().raw(), "no balance object found" );
   eosio_assert( from.balance.amount >= value.amount, "overdrawn balance" );

   from_acnts.modify( from, owner, [&]( auto& a ) {
         a.balance -= value;
      });
}

void patreosvault::add_balance( name owner, asset value, name ram_payer )
{
   accounts to_acnts( _self, owner.value );
   auto to = to_acnts.find( value.symbol.code().raw() );
   if( to == to_acnts.end() ) {
      to_acnts.emplace( ram_payer, [&]( auto& a ){
        a.balance = value;
      });
   } else {
      to_acnts.modify( to, same_payer, [&]( auto& a ) {
        a.balance += value;
      });
   }
}

void patreosvault::transferAction( name self, name code ) {
    auto data = unpack_action_data<transfer>();
    //if(data.from.value == self || data.to.value != self)
    //    return;

    /*
    eosio::print(">>> data.quantity.amount value is ______\n", data.quantity.amount);
    eosio::print(">>> data.quantity.symbol value is ______\n", data.quantity.symbol);
    eosio::print(">>> self.value is ", self);
    eosio::print("\n");
    eosio::print(">>> code is ", code);
    eosio::print(", but should be ", EOS_TOKEN_CODE);
    eosio::print("\n");
    eosio::print(">>> code.value is ", code.value);
    eosio::print(", but should be ", EOS_TOKEN_CODE.value);
    eosio::print("\n");
    */


    bool valid_deposit = false;
    if(code.value == EOS_TOKEN_CODE.value && data.quantity.symbol == EOS_SYMBOL) {
       valid_deposit = true;
       eosio_assert(data.quantity.amount >= eos_fee.amount, "Minimum deposit of 0.1 EOS required");
    }
    if(code.value == PATREOS_TOKEN_CODE.value && data.quantity.symbol == PTR_SYMBOL) {
      valid_deposit = true;
      eosio_assert(data.quantity.amount >= patreos_fee.amount, "Minimum deposit of 50 PTR required");
    }


    eosio_assert(valid_deposit, "We currently do not support this token");
    eosio_assert(data.quantity.is_valid(), "Invalid quantity");
    eosio_assert(data.quantity.amount > 0, "Cannot transfer non-positive amount");

    print("Thank you for your deposit!");
    add_balance(data.from, data.quantity, _self);
}


void patreosvault::withdraw( name account, asset quantity ) {
  require_auth( account );
}


// Process outstanding subscriptions
void patreosvault::process( name processor, name from, name to, asset quantity )
{
    eosio_assert(has_auth(to) || has_auth(PATREOS_NEXUS_CODE),
      "Not authorized to process this subscription");
    eosio_assert( is_supported_asset(quantity), "We do not support this token currently");

    // Find pledge in patreosnexus
    auto pledgestable = patreosvault::pledges(PATREOS_NEXUS_CODE, from.value);
    auto existing = pledgestable.find(to.value);
    eosio_assert( existing != pledgestable.end(), "Pledge does not exist." );

    // Check that action asset is valid and consistent with pledge asset
    eosio_assert( quantity.is_valid(), "invalid quantity" );
    eosio_assert( quantity.amount > 0, "must transfer positive quantity" );
    eosio_assert( quantity.symbol == existing->quantity.symbol, "symbol precision mismatch" );

    // Check from account has the funds
    accounts from_acnts( _self, from.value );
    const auto& match = from_acnts.get( quantity.symbol.code().raw(), "no balance object found" );
    if(match.balance.amount < quantity.amount) {
      // Cancel pledge in patreosnexus due to insufficent funds, someone is naughty
      action(permission_level{ _self, EOS_CODE_PERMISSION },
          PATREOS_NEXUS_CODE, "depledge"_n,
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
      action(permission_level{ _self, EOS_CODE_PERMISSION },
          PATREOS_NEXUS_CODE, "pledge_paid"_n,
          std::make_tuple(from, to)).send();
    }
}

extern "C" void apply(uint64_t receiver, uint64_t code, uint64_t action) {
    size_t size = action_data_size();
    //using malloc/free here potentially is not exception-safe, although WASM doesn't support exceptions
    constexpr size_t max_stack_buffer_size = 512;
    void* buffer = nullptr;
    if( size > 0 ) {
      buffer = max_stack_buffer_size < size ? malloc(size) : alloca(size);
      read_action_data( buffer, size );
    }
    datastream<const char*> ds((char*)buffer, size);
    patreosvault thiscontract( eosio::name(receiver), eosio::name(code), ds );

    if (code == receiver) { // Calling our contract actions
      switch (action) {
        EOSIO_DISPATCH_HELPER(patreosvault, (process)(withdraw))
      }
      /* does not allow destructor of thiscontract to run: eosio_exit(0); */
    } else if(action == ("transfer"_n).value) { // Another contract making a transfer to contract account
      eosio::print( ">>> patreosvault has a transfer!\n" );

      /*
      if(code == EOS_TOKEN_CODE.value || code == PATREOS_TOKEN_CODE.value) {
         eosio::execute_action( eosio::name(receiver), eosio::name(code), &patreosvault::transferAction );
      }
      */
      return thiscontract.transferAction(eosio::name(receiver), eosio::name(code));
      //eosio::execute_action( eosio::name(receiver), eosio::name(code), &patreosvault::transferAction );
    } else {
      eosio_exit(0);
    }
}

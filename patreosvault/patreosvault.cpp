#include <eosiolib/eosio.hpp>
#include <eosiolib/asset.hpp>
#include <eosiolib/action.hpp>
#include <eosiolib/multi_index.hpp>
#include "patreosvault.hpp"
#include "../common/messages.hpp"

using namespace eosio;

void patreosvault::sub_balance( name owner, asset value ) {
   accounts from_acnts( _self, owner.value );

   const auto& from = from_acnts.get( value.symbol.code().raw(), Messages::NO_BALANCE_OBJECT );
   eosio_assert( from.balance.amount >= value.amount, Messages::OVERDRAWN_BALANCE );

   from_acnts.modify( from, owner, [&]( auto& a ) {
     a.balance -= value;
   });
}

void patreosvault::add_balance( name owner, asset value, name ram_payer )
{
   print("Adding balance to ", owner); print("\n");
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
    if(data.from.value == self.value || data.to.value != self.value) {
      // Ignore outgoing transactions (withdraws)
      return;
    }

    print(">>> self is: ", self); print("\n");
    print(">>> code is: ", code); print("\n");
    print(">>> data.from is: ", data.from); print("\n");
    print(">>> data.to is: ", data.to); print("\n");
    print(">>> data.quantity.symbol is: ", data.quantity.symbol); print("\n");
    print(">>> data.quantity.amount is: ", data.quantity.amount); print("\n");

    bool valid_deposit = false;
    if(code.value == EOS_TOKEN_CODE.value && data.quantity.symbol == EOS_SYMBOL) {
      eosio_assert(data.quantity.amount >= eos_fee.amount, Messages::NEED_MIN_EOS_DEPOSIT);
      valid_deposit = true;
    }
    if(code.value == PATREOS_TOKEN_CODE.value && data.quantity.symbol == PTR_SYMBOL) {
      eosio_assert(data.quantity.amount >= patreos_fee.amount, Messages::NEED_MIN_PATR_DEPOSIT);
      valid_deposit = true;
    }

    eosio_assert(valid_deposit, Messages::UNSUPPORTED_TOKEN);
    eosio_assert(data.quantity.is_valid(), Messages::INVALID_QUANTITY);
    eosio_assert(data.quantity.amount > 0, Messages::NEED_POSITIVE_TRANSFER_AMOUNT);

    // Reference contract token stats
    auto sym = data.quantity.symbol.code().raw();
    stats statstable( code, sym );
    const auto& st = statstable.get( sym );
    eosio_assert( data.quantity.symbol == st.supply.symbol, Messages::INVALID_SYMBOL );

    print(">>> Thank you for your deposit!\n");
    add_balance(data.from, data.quantity, _self);
}


void patreosvault::withdraw( name owner, asset quantity ) {
  require_auth( owner );
  accounts _accounts( _self, owner.value );

  const auto& from = _accounts.get( quantity.symbol.code().raw(), Messages::NO_BALANCE_FOR_TOKEN ); // Nice try
  eosio_assert( from.balance.amount >= quantity.amount, Messages::OVERDRAWN_BALANCE );
  eosio_assert( quantity.is_valid(), Messages::INVALID_QUANTITY );
  eosio_assert( quantity.amount > 0, Messages::NEED_POSITIVE_TRANSFER_QUANTITY );

  name token_code;
  bool defer = false;
  if(quantity.symbol == EOS_SYMBOL) {
    token_code = EOS_TOKEN_CODE;
  } else if (quantity.symbol == PTR_SYMBOL) {
    token_code = PATREOS_TOKEN_CODE;
    defer = true;
  } else {
    eosio_assert( false, Messages::TOKEN_CONTRACT_DNE );
  }

  auto sym = quantity.symbol.code();
  stats statstable( token_code, sym.raw() );
  const auto& st = statstable.get( sym.raw() );
  eosio_assert( quantity.symbol == st.supply.symbol, Messages::INVALID_SYMBOL );

  sub_balance(owner, quantity);

  // If PATR, defer transaction 3 days, but consider issues with deferred tx
  action(permission_level{ PATREOS_VAULT_CODE, EOS_ACTIVE_PERMISSION },
      token_code, EOS_TRANSFER_ACTION,
      std::make_tuple(_self, owner, quantity, std::string("Here's your money back"))).send();
}


// Process outstanding subscriptions
void patreosvault::process( name processor, name from, name to, asset quantity )
{
    bool authorized = has_auth(to) || has_auth(PATREOS_NEXUS_CODE);
    eosio_assert(authorized, Messages::NEED_AUTH_FOR_PROCESSING);
    eosio_assert( is_supported_asset(quantity), Messages::UNSUPPORTED_TOKEN);

    // Find pledge in patreosnexus
    auto pledgestable = patreosvault::pledges(PATREOS_NEXUS_CODE, from.value);
    auto existing = pledgestable.find(to.value);
    eosio_assert( existing != pledgestable.end(), Messages::PLEDGE_DNE );

    // Check that action asset is valid and consistent with pledge asset
    eosio_assert( quantity.is_valid(), Messages::INVALID_QUANTITY );
    eosio_assert( quantity.amount > 0, Messages::NEED_POSITIVE_TRANSFER_QUANTITY );
    eosio_assert( quantity.symbol == existing->quantity.symbol, Messages::INVALID_SYMBOL );

    // Check from account has the funds
    accounts from_acnts( _self, from.value );
    const auto& match = from_acnts.get( quantity.symbol.code().raw(), Messages::NO_BALANCE_OBJECT );
    if(match.balance.amount < quantity.amount) {
      // Cancel pledge in patreosnexus due to insufficent funds, someone is naughty
      action(permission_level{ _self, EOS_ACTIVE_PERMISSION },
          PATREOS_NEXUS_CODE, PATREOS_NEXUS_DEPLEDGE_ACTION,
          std::make_tuple(from, to)).send();
    } else {
      // Verify subscription due date
      double milliseconds_since_last_pledge = double(now() - existing->last_pledge);
      int seconds_since_last_pledge = (int) ( milliseconds_since_last_pledge / 1000 );
      eosio_assert( existing->seconds <= seconds_since_last_pledge, Messages::PLEDGE_NOT_DUE );

      require_recipient( from );
      require_recipient( to );

      asset fee;
      if(quantity.symbol == EOS_SYMBOL) {
        fee = eos_fee;
      } else if (quantity.symbol == PTR_SYMBOL) {
        fee = patreos_fee;
      } else {
        eosio_assert( false, Messages::TOKEN_FEE_DNE );
      }

      // execute subscription
      sub_balance(from, quantity);
      add_balance(to, quantity - fee, _self);
      add_balance(processor, fee, _self); // fee to processor

      // Increment execution_count and update last_pledge
      action(permission_level{ _self, EOS_ACTIVE_PERMISSION },
          PATREOS_NEXUS_CODE, PATREOS_NEXUS_PLEDGEPAID_ACTION,
          std::make_tuple(from, to)).send();
    }
}

extern "C" void apply(uint64_t receiver, uint64_t code, uint64_t action) {
    if (code == receiver) { // Calling our contract actions
      switch (action) {
        EOSIO_DISPATCH_HELPER(patreosvault, (process)(withdraw))
      }
      /* does not allow destructor of thiscontract to run: eosio_exit(0); */
    } else if(action == EOS_TRANSFER_ACTION.value) { // External transfer to patreosvault
      eosio::print( ">>> patreosvault has a transfer!\n" );

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

      return thiscontract.transferAction(eosio::name(receiver), eosio::name(code));
    } else {
      eosio_exit(0);
    }
}

#include <eosiolib/eosio.hpp>
#include <eosiolib/asset.hpp>
#include <eosiolib/action.hpp>
#include <eosiolib/multi_index.hpp>
#include "include/recurringpay.hpp"
#include "include/messages.hpp"

using namespace eosio;


void recurringpay::add_balance( name owner, name contract, asset quantity, name ram_payer )
{
   print(" Token contract is ", contract);
   print(" Adding balance to ", owner); print("\n");
   balances balances_table( _self, owner.value );
   auto balances_table_secondary = balances_table.get_index<"code.symbol"_n>();
   auto token_contract_id = combine_ids(contract.value, quantity.symbol.code().raw());
   print(" token_contract_id is: ", token_contract_id);
   eosio_assert(quantity.amount > 0, "Quantity cannot be negative");

   auto to = balances_table_secondary.find( token_contract_id );
   if( to == balances_table_secondary.end() ) {
      balances_table.emplace( ram_payer, [&]( auto& a ){
        a.id = balances_table.available_primary_key();
        a.contract = contract;
        a.quantity = quantity;
      });
   } else {
      balances_table_secondary.modify( to, same_payer, [&]( auto& a ) {
        a.quantity += quantity;
      });
   }
}

void recurringpay::sub_balance( name owner, name contract, asset quantity ) {
   print(" Token contract is ", contract);
   print(" Subtracting balance from ", owner); print("\n");
   balances balances_table( _self, owner.value );
   auto balances_table_secondary = balances_table.get_index<"code.symbol"_n>();

   auto token_contract_id = combine_ids(contract.value, quantity.symbol.code().raw());
   auto from = balances_table_secondary.find( token_contract_id );
   eosio_assert(from != balances_table_secondary.end(), Messages::NO_BALANCE_OBJECT);
   eosio_assert(quantity.amount > 0, "Quantity cannot be negative");
   eosio_assert( from->quantity.amount >= quantity.amount, Messages::OVERDRAWN_BALANCE );

   balances_table_secondary.modify( from, owner, [&]( auto& a ) {
     a.quantity -= quantity;
   });
}

void recurringpay::execute_subscription( name provider, name from, name to,
  name contract, asset quantity, asset fee )
{
  sub_balance(
    from,
    contract,
    quantity
  );
  add_balance(
    to,
    contract,
    quantity - fee,
    _self
  );
  // Service provider gets fee
  add_balance(
    provider,
    contract,
    fee,
    _self
  );
}

void recurringpay::regservice( name provider, vector<raw_token_service_stat> valid_tokens )
{
   require_auth( provider );

   print("Registering Subscription Service Provider ", provider); print("\n");
   services services_table( _self, provider.value );
   auto services_table_secondary = services_table.get_index<"code.symbol"_n>();

   // Check that token profiles are unique
   for(auto it = valid_tokens.begin(); it != valid_tokens.end(); it++ ) {
     uint64_t token_contract_id = it->token_contract.value;
     uint64_t token_symbol_id = it->flat_fee.symbol.code().raw();
     uint128_t code_and_symbol_id = combine_ids(token_contract_id, token_symbol_id);
     auto token_service_stat_itr = services_table_secondary.find( code_and_symbol_id );
     if(token_service_stat_itr != services_table_secondary.end()) {
       print("Service provider already accepts this token.  Skipping.\n");
       continue;
     }

     services_table.emplace( provider, [&]( auto& s ){
       s.id = services_table.available_primary_key();
       s.token_contract = it->token_contract;
       s.flat_fee = it->flat_fee;
       s.percentage_fee = it->percentage_fee;
     });
   }
}

// Allow service provider to update subscription fees, increase at your own risk
void recurringpay::updatefee( name provider, name from, name to, asset fee ) {
  require_auth( provider );

  agreements agreements_table( _self, provider.value );
  auto agreements_table_secondary = agreements_table.get_index<"from.to"_n>();
  auto party_agreement_id = combine_ids(from.value, to.value);
  auto agreement_itr = agreements_table_secondary.find( party_agreement_id );

  eosio_assert(agreement_itr != agreements_table_secondary.end(), "Agreement not found!");
  eosio_assert(agreement_itr->token_profile_amount.quantity > fee, "Fees cannot be more than subscription quantity");

  // TODO: CHECK symbol

  agreements_table_secondary.modify( agreement_itr, same_payer, [&]( auto& s ) {
    s.fee = fee;
  });
}

void recurringpay::subscribe( name provider, recurringpay::raw_agreement agreement ) {
  require_auth(agreement.from);
  eosio_assert( is_account( agreement.to ), "Receiving account does not exist" );

  uint64_t token_contract_id = agreement.token_profile_amount.contract.value;
  uint64_t token_symbol_id = agreement.token_profile_amount.quantity.symbol.code().raw();
  uint128_t code_and_symbol_id = combine_ids(token_contract_id, token_symbol_id);

  // Secondary index search
  services services_table( _self, provider.value );
  auto services_table_secondary = services_table.get_index<"code.symbol"_n>();
  auto token_service_stat_itr = services_table_secondary.find( code_and_symbol_id );

  eosio_assert(token_service_stat_itr != services_table_secondary.end(), "Service provider doesn't accept this token");

  // TODO: Check provider token symbol == flat_fee symbol == subscription symbol

  // Adjust fee logic by percentage
  float percentage_fee = token_service_stat_itr->percentage_fee / 100.00;
  asset flat_fee = token_service_stat_itr->flat_fee;
  eosio_assert(percentage_fee <= 1, "Percentage fee cannot be more than 100");

  asset percentage_fee_asset = asset(
    (uint64_t) ( agreement.token_profile_amount.quantity.amount * percentage_fee ),
    agreement.token_profile_amount.quantity.symbol
  );
  eosio_assert(percentage_fee_asset <= agreement.token_profile_amount.quantity, "Percentage fee cannot be more than subscription amount");

  asset final_fee = flat_fee + percentage_fee_asset;
  print("subscription is: ", agreement.token_profile_amount.quantity); print("\n");
  print("final fee is: ", final_fee); print("\n");
  eosio_assert( final_fee <= agreement.token_profile_amount.quantity, "Percentage fee cannot be more than subscription amount");

  // Verify agreement doesn't already exist
  agreements agreements_table( _self, provider.value );
  auto agreements_table_secondary = agreements_table.get_index<"from.to"_n>();

  uint128_t party_agreement_id = combine_ids(agreement.from.value, agreement.to.value);
  auto agreement_itr = agreements_table_secondary.find( party_agreement_id );
  eosio_assert(agreement_itr == agreements_table_secondary.end(), "Agreement already exists!");

  eosio_assert(is_pledge_cycle_valid(agreement.cycle_seconds), "Subscription cycle not valid!");

  // TODO: CHECK QUANITTY VALID, AMOUNT POSITIVE, etc

  print("Adding agreement with secondary key (from.to): ", party_agreement_id); print("\n");
  print("Adding agreement with secondary key: (from): ", agreement.from.value); print("\n");

  execute_subscription(
    provider,
    agreement.from,
    agreement.to,
    agreement.token_profile_amount.contract,
    agreement.token_profile_amount.quantity,
    final_fee
  );

  agreements_table.emplace( agreement.from, [&]( auto& a ){
    a.id = agreements_table.available_primary_key();
    a.from = agreement.from;
    a.to = agreement.to;
    a.token_profile_amount.contract = agreement.token_profile_amount.contract;
    a.token_profile_amount.quantity = agreement.token_profile_amount.quantity;
    a.cycle_seconds = agreement.cycle_seconds;
    a.last_executed = now();
    a.execution_count = 1;
    a.fee = final_fee;
  });

}

// Open to world, anyone can process
void recurringpay::process( name provider, name from, name to ) {
  agreements agreements_table( _self, provider.value );
  auto agreements_table_secondary = agreements_table.get_index<"from.to"_n>();

  uint128_t party_agreement_id = combine_ids(from.value, to.value);
  auto agreement_itr = agreements_table_secondary.find( party_agreement_id );

  uint64_t date_in_seconds = now();
  print("Now is ", date_in_seconds); print("\n");
  print("Last executed is ", agreement_itr->last_executed); print("\n");
  print("Cycle is ", agreement_itr->cycle_seconds); print("\n");

  eosio_assert(agreement_itr != agreements_table_secondary.end(), "Agreement does not exists!");

  eosio_assert(agreement_itr->cycle_seconds > 0, "Invalid cycle!"); // Don't break math
  eosio_assert(agreement_itr->last_executed <= date_in_seconds, "Invalid last execution date!"); // No time travel allowed
  eosio_assert(agreement_itr->last_executed > 1545699988, "Invalid last execution date!"); // Again, no time travelling

  uint64_t payments_due = ( date_in_seconds - agreement_itr->last_executed ) / agreement_itr->cycle_seconds;

  // TODO: execute subscriptions based on number of payments due
  eosio_assert(payments_due > 1, "Subscription is not due");

  print("Number of due payments is ", payments_due); print("\n");

  execute_subscription(
    provider,
    agreement_itr->from,
    agreement_itr->to,
    agreement_itr->token_profile_amount.contract,
    agreement_itr->token_profile_amount.quantity,
    agreement_itr->fee
  );

  agreements_table_secondary.modify( agreement_itr, same_payer, [&]( auto& s ) {
    s.last_executed = date_in_seconds;
    s.execution_count++;
  });
}

void recurringpay::unsubscribe( name provider, name from, name to ) {
  require_auth(from);

  // TODO: Clear out pending subscriptions

  agreements agreements_table( _self, provider.value ); // table scoped by provider
  auto agreements_table_secondary = agreements_table.get_index<"from.to"_n>();

  uint128_t party_agreement_id = combine_ids(from.value, to.value);
  auto agreement_itr = agreements_table_secondary.find( party_agreement_id );
  eosio_assert(agreement_itr != agreements_table_secondary.end(), "Agreement does not exist!");
  agreement_itr = agreements_table_secondary.erase(agreement_itr);
}

void recurringpay::alert( name to, string memo ) {
  eosio_assert( is_account( to ), Messages::TO_ACCCOUNT_DNE );
  require_recipient( to );
}

void recurringpay::transferAction( name self, name code ) {
    auto data = unpack_action_data<transfer>();
    if(data.from.value == self.value || data.to.value != self.value) {
      // Ignore outgoing transactions (withdraws)
      // TODO: return tokens?
      return;
    }

    // TODO: give regcredit if memo format exists, and deposit is correct

    print(">>> self is: ", self); print("\n");
    print(">>> code is: ", code); print("\n");
    print(">>> data.from is: ", data.from); print("\n");
    print(">>> data.to is: ", data.to); print("\n");
    print(">>> data.quantity.symbol is: ", data.quantity.symbol); print("\n");
    print(">>> data.quantity.amount is: ", data.quantity.amount); print("\n");

    eosio_assert(data.quantity.is_valid(), Messages::INVALID_QUANTITY);
    eosio_assert(data.quantity.amount > 0, Messages::NEED_POSITIVE_TRANSFER_AMOUNT);

    // Reference contract token stats
    auto sym = data.quantity.symbol.code().raw();
    stats statstable( code, sym );
    const auto& st = statstable.get( sym );
    eosio_assert( data.quantity.symbol == st.supply.symbol, Messages::INVALID_SYMBOL );

    print(">>> Thank you for your deposit!\n");

    // TODO: Check contract for our actual balance, if not found then do not add
    add_balance(data.from, code, data.quantity, _self);
}


void recurringpay::withdraw( name owner, name contract, asset quantity ) {
  require_auth( owner );

  balances balances_table( _self, owner.value );
  auto balances_table_secondary = balances_table.get_index<"code.symbol"_n>();
  auto token_contract_id = combine_ids(contract.value, quantity.symbol.code().raw());

  auto from = balances_table_secondary.find( token_contract_id );
  eosio_assert(from != balances_table_secondary.end(), "No balance found!");

  eosio_assert( from->quantity >= quantity, Messages::OVERDRAWN_BALANCE );
  eosio_assert( quantity.is_valid(), Messages::INVALID_QUANTITY );
  eosio_assert( quantity.amount > 0, Messages::NEED_POSITIVE_TRANSFER_QUANTITY );

  auto sym = quantity.symbol.code();
  stats statstable( contract, sym.raw() );
  const auto& st = statstable.get( sym.raw() );
  eosio_assert( quantity.symbol == st.supply.symbol, Messages::INVALID_SYMBOL );

  sub_balance(owner, contract, quantity);

  // TODO: Check contract for our actual balance, if not found then remove from our balances_table

  action(permission_level{ RECURRING_PAY_CODE, EOS_ACTIVE_PERMISSION },
      contract, EOS_TRANSFER_ACTION,
      std::make_tuple(_self, owner, quantity, std::string("Here's your money back"))).send();
}

extern "C" void apply(uint64_t receiver, uint64_t code, uint64_t action) {
    if (code == receiver) { // Calling our contract actions
      switch (action) {
        EOSIO_DISPATCH_HELPER(recurringpay, (regservice)(withdraw)(updatefee)(subscribe)(unsubscribe)(process))
      }
      /* does not allow destructor of thiscontract to run: eosio_exit(0); */
    } else if(action == EOS_TRANSFER_ACTION.value) { // External transfer to recurringpay
      eosio::print( ">>> recurringpay has a transfer!\n" );

      size_t size = action_data_size();
      //using malloc/free here potentially is not exception-safe, although WASM doesn't support exceptions
      constexpr size_t max_stack_buffer_size = 512;
      void* buffer = nullptr;
      if( size > 0 ) {
        buffer = max_stack_buffer_size < size ? malloc(size) : alloca(size);
        read_action_data( buffer, size );
      }
      datastream<const char*> ds((char*)buffer, size);
      recurringpay thiscontract( eosio::name(receiver), eosio::name(code), ds );

      return thiscontract.transferAction(eosio::name(receiver), eosio::name(code));
    } else {
      eosio_exit(0);
    }
}

#include <eosiolib/eosio.hpp>
#include <eosiolib/asset.hpp>
#include <eosiolib/action.hpp>
#include <eosiolib/multi_index.hpp>
#include "patreospayer.hpp"
#include "../common/messages.hpp"

using namespace eosio;


void patreospayer::add_balance( name owner, name contract, asset quantity, name ram_payer )
{
   print(" Token contract is ", contract);
   print(" Adding balance to ", owner); print("\n");
   balances vault_balances( _self, owner.value );
   auto vault_balances_secondary = vault_balances.get_index<"code.symbol"_n>();
   auto token_contract_id = combine_ids(contract.value, quantity.symbol.code().raw());

   auto to = vault_balances_secondary.find( token_contract_id );
   if( to == vault_balances_secondary.end() ) {
      print(" FIRST TIME!!! ");
      vault_balances.emplace( ram_payer, [&]( auto& a ){
        a.id = vault_balances.available_primary_key();
        a.contract = contract;
        a.quantity = quantity;
      });
   } else {
      print(" JUST UPDATING ");
      vault_balances_secondary.modify( to, same_payer, [&]( auto& a ) {
        a.quantity += quantity;
      });
   }
}

void patreospayer::sub_balance( name owner, name contract, asset quantity ) {
   print(" Token contract is ", contract);
   print(" Subtracting balance from ", owner); print("\n");
   balances vault_balances( _self, owner.value );
   auto vault_balances_secondary = vault_balances.get_index<"code.symbol"_n>();

   auto token_contract_id = combine_ids(contract.value, quantity.symbol.code().raw());
   auto from = vault_balances_secondary.find( token_contract_id );
   eosio_assert(from != vault_balances_secondary.end(), Messages::NO_BALANCE_OBJECT);
   eosio_assert( from->quantity.amount >= quantity.amount, Messages::OVERDRAWN_BALANCE );

   vault_balances_secondary.modify( from, owner, [&]( auto& a ) {
     a.quantity -= quantity;
   });
}

void patreospayer::regservice( name provider, vector<raw_token_service_stat> valid_tokens )
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
void patreospayer::updatefee( name provider, name from, name to, asset fee ) {
  require_auth( provider );

  agreements agreements_table( _self, provider.value );
  auto party_agreement_id = combine_ids(from.value, to.value);
  auto agreement = agreements_table.find( party_agreement_id );

  eosio_assert(agreement != agreements_table.end(), "Agreement not found!");

  // TODO: CHECK symbol

  agreements_table.modify( agreement, same_payer, [&]( auto& s ) {
    s.fee = fee;
  });
}

void patreospayer::subscribe( name provider, patreospayer::raw_agreement agreement ) {
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

  // After fee logic, verify token_service_stat is unique
  eosio_assert(++token_service_stat_itr == services_table_secondary.end(), "Service provider should have unique token profiles!");

  // Verify agreement doesn't already exist
  agreements agreements_table( _self, provider.value );
  auto agreements_table_secondary = agreements_table.get_index<"from.to"_n>();

  uint128_t party_agreement_id = combine_ids(agreement.from.value, agreement.to.value);
  auto agreement_itr = agreements_table_secondary.find( party_agreement_id );
  eosio_assert(agreement_itr == agreements_table_secondary.end(), "Agreement already exists!");

  eosio_assert(is_pledge_cycle_valid(agreement.cycle), "Subscription cycle not valid!");

  // TODO: CHECK QUANITTY VALID, AMOUNT POSITIVE, etc

  // CHANGE RAM OWNERSHIP OF TOKEN DEPOSIT

  print("Adding agreement with secondary key (from.to): ", party_agreement_id); print("\n");
  print("Adding agreement with secondary key: (from): ", agreement.from.value); print("\n");

  agreements_table.emplace( agreement.from, [&]( auto& a ){
    a.id = agreements_table.available_primary_key();
    a.from = agreement.from;
    a.to = agreement.to;
    a.token_profile_amount.contract = agreement.token_profile_amount.contract;
    a.token_profile_amount.quantity = agreement.token_profile_amount.quantity;
    a.cycle = agreement.cycle;
    a.last_executed = now();
    a.execution_count = 0;
    a.fee = final_fee;
  });
}

// Open to world, anyone can process
void patreospayer::process( name provider, name from, name to ) {
  agreements agreements_table( _self, provider.value );
  auto agreements_table_secondary = agreements_table.get_index<"from.to"_n>();

  uint128_t party_agreement_id = combine_ids(from.value, to.value);
  auto agreement_itr = agreements_table_secondary.find( party_agreement_id );

  print("Now is ", now()); print("\n");
  print("Last executed is ", agreement_itr->last_executed); print("\n");
  print("Cycle is ", agreement_itr->cycle); print("\n");

  eosio_assert(agreement_itr != agreements_table_secondary.end(), "Agreement does not exists!");
  eosio_assert(now() > agreement_itr->last_executed + agreement_itr->cycle, "Subscription is not due!");

  sub_balance(
    agreement_itr->from,
    agreement_itr->token_profile_amount.contract,
    agreement_itr->token_profile_amount.quantity
  );
  add_balance(
    agreement_itr->to,
    agreement_itr->token_profile_amount.contract,
    agreement_itr->token_profile_amount.quantity - agreement_itr->fee,
    _self
  );
  // Service provider gets fee
  add_balance(
    provider,
    agreement_itr->token_profile_amount.contract,
    agreement_itr->fee,
    _self
  );

}

void patreospayer::unsubscribe( name provider, name from, name to ) {
  require_auth(from);

  // TODO: Clear out pending subscriptions

  agreements agreements_table( _self, provider.value ); // table scoped by provider
  auto agreements_table_secondary = agreements_table.get_index<"from.to"_n>();

  uint128_t party_agreement_id = combine_ids(from.value, to.value);
  auto agreement_itr = agreements_table_secondary.find( party_agreement_id );
  eosio_assert(agreement_itr != agreements_table_secondary.end(), "Agreement does not exist!");
  agreement_itr = agreements_table_secondary.erase(agreement_itr);
  eosio_assert(agreement_itr == agreements_table_secondary.end(), "Agreement should have unique entry!");
}

void patreospayer::transferAction( name self, name code ) {
    auto data = unpack_action_data<transfer>();
    if(data.from.value == self.value || data.to.value != self.value) {
      // Ignore outgoing transactions (withdraws)
      // TODO: return tokens?
      return;
    }

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
    add_balance(data.from, code, data.quantity, _self);
}


void patreospayer::withdraw( name owner, asset quantity ) {
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

  //sub_balance(owner, quantity);

  // If PATR, defer transaction 3 days, but consider issues with deferred tx
  action(permission_level{ PATREOS_VAULT_CODE, EOS_ACTIVE_PERMISSION },
      token_code, EOS_TRANSFER_ACTION,
      std::make_tuple(_self, owner, quantity, std::string("Here's your money back"))).send();
}

extern "C" void apply(uint64_t receiver, uint64_t code, uint64_t action) {
    if (code == receiver) { // Calling our contract actions
      switch (action) {
        EOSIO_DISPATCH_HELPER(patreospayer, (regservice)(withdraw)(updatefee)(subscribe)(unsubscribe)(process))
      }
      /* does not allow destructor of thiscontract to run: eosio_exit(0); */
    } else if(action == EOS_TRANSFER_ACTION.value) { // External transfer to patreospayer
      eosio::print( ">>> patreospayer has a transfer!\n" );

      size_t size = action_data_size();
      //using malloc/free here potentially is not exception-safe, although WASM doesn't support exceptions
      constexpr size_t max_stack_buffer_size = 512;
      void* buffer = nullptr;
      if( size > 0 ) {
        buffer = max_stack_buffer_size < size ? malloc(size) : alloca(size);
        read_action_data( buffer, size );
      }
      datastream<const char*> ds((char*)buffer, size);
      patreospayer thiscontract( eosio::name(receiver), eosio::name(code), ds );

      return thiscontract.transferAction(eosio::name(receiver), eosio::name(code));
    } else {
      eosio_exit(0);
    }
}

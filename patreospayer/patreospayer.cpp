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
   auto token_contract_id = combine_ids(contract.value, quantity.symbol.code().raw());

   auto to = vault_balances.find( token_contract_id );
   if( to == vault_balances.end() ) {
      vault_balances.emplace( ram_payer, [&]( auto& a ){
        a.contract = contract;
        a.quantity = quantity;
      });
   } else {
      vault_balances.modify( to, same_payer, [&]( auto& a ) {
        a.quantity += quantity;
      });
   }
}

void patreospayer::sub_balance( name owner, name contract, asset quantity ) {
   print(" Token contract is ", contract);
   print(" Subtracting balance from ", owner); print("\n");
   balances vault_balances( _self, owner.value );

   auto token_contract_id = combine_ids(contract.value, quantity.symbol.code().raw());
   const auto& from = vault_balances.get( token_contract_id, Messages::NO_BALANCE_OBJECT );
   eosio_assert( from.quantity.amount >= quantity.amount, Messages::OVERDRAWN_BALANCE );

   vault_balances.modify( from, owner, [&]( auto& a ) {
     a.quantity -= quantity;
   });
}

void patreospayer::regservice( name provider, vector<vault_token> valid_tokens )
{
   require_auth( provider );

   print("Registering Subscription Service Provider ", provider); print("\n");
   services t_services( _self, _self.value );
   auto service = t_services.find( provider.value );
   if( service == t_services.end() ) {
      t_services.emplace( provider, [&]( auto& s ){
        s.provider = provider;
        s.valid_tokens = valid_tokens;
      });
   } else {
      t_services.modify( service, same_payer, [&]( auto& s ) {
        s.valid_tokens = valid_tokens;
      });
   }
}

void patreospayer::updatefee( name provider, name from, name to, asset fee ) {
  require_auth( provider );

  agreements t_agreements( _self, provider.value );
  auto party_agreement_id = combine_ids(from.value, to.value);
  auto agreement = t_agreements.find( party_agreement_id );

  // TODO: CHECK symbol

  eosio_assert(agreement != t_agreements.end(), "Agreement not found!");
  t_agreements.modify( agreement, same_payer, [&]( auto& s ) {
    s.fee = fee;
  });

}

void patreospayer::subscribe( name provider, patreospayer::agreement _agreement ) {
  require_auth(_agreement.from);
  eosio_assert( is_account( _agreement.to ), "To party account does not exist" );

  services t_services( _self, _self.value );
  const auto& service = t_services.get( provider.value, "Service provider not found");

  asset subscription_fee;
  bool subscription_fee_found = false;
  std::vector<patreospayer::vault_token> valid_tokens = service.valid_tokens;
  for(auto it = valid_tokens.begin(); it != valid_tokens.end(); it++ )    {
      bool contract_match = (*it).contract.value == _agreement.vault_token_amount.contract.value;
      bool symbol_match = (*it).quantity.symbol == _agreement.vault_token_amount.quantity.symbol;
      if(contract_match && symbol_match) {
        subscription_fee_found = true;
        subscription_fee = (*it).quantity;
      }
  }
  eosio_assert( subscription_fee_found, "Service provider does not accept your token" );

  print(" Fee found is ", subscription_fee);
  agreements t_agreements( _self, provider.value ); // table scoped by provider

  auto party_agreement_id = combine_ids(_agreement.from.value, _agreement.to.value);
  auto agreement = t_agreements.find( party_agreement_id );
  eosio_assert(agreement == t_agreements.end(), "Agreement already exists!");

  // TO VERIFY cycle

  // TODO: CHECK QUANITTY VALID, AMOUNT POSITIVE, etc
  t_agreements.emplace( _agreement.from, [&]( auto& a ){
    a = _agreement;
    a.last_executed = 0;
    a.fee = subscription_fee;
  });
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
    add_balance(data.from, code, data.quantity, data.from);
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
        EOSIO_DISPATCH_HELPER(patreospayer, (regservice)(withdraw)(updatefee)(subscribe))
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

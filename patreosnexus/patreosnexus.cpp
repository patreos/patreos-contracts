#include "patreosnexus.hpp"
#include "../common/messages.hpp"

using namespace eosio;

void patreosnexus::follow( name owner, name following )
{
  require_auth(owner);
  require_recipient( following );
}

void patreosnexus::unfollow( name owner, name following )
{
  require_auth(owner);
  require_recipient( following ); // Notify on unfollow?
}

void patreosnexus::pledge( name pledger, patreosnexus::pledge_data _pledge )
{
  require_auth(pledger);
  eosio_assert( is_account( _pledge.creator ), Messages::CREATOR_ACCOUNT_DNE );

  // Verify pledge doesn't exist already
  pledges pledgetable( _self, pledger.value ); // table scoped by pledger
  auto match = pledgetable.find( _pledge.creator.value );
  eosio_assert( match == pledgetable.end(), Messages::PLEDGE_EXISTS );

  // Check pledge quantity
  eosio_assert( is_supported_asset(_pledge.quantity), Messages::UNSUPPORTED_TOKEN );
  eosio_assert( _pledge.quantity.is_valid(), Messages::INVALID_QUANTITY );
  eosio_assert( _pledge.quantity.amount > 0, Messages::NEED_POSITIVE_PLEDGE_QUANTITY );

  asset min_quantity;
  name token_code;
  if(_pledge.quantity.symbol == EOS_SYMBOL) {
    min_quantity = min_pledge_eos;
    token_code = EOS_TOKEN_CODE;
  } else if (_pledge.quantity.symbol == PTR_SYMBOL) {
    min_quantity = min_pledge_ptr;
    token_code = PATREOS_TOKEN_CODE;
  } else {
    eosio_assert( false, Messages::UNFOUND_TOKEN );
  }
  eosio_assert( _pledge.quantity.amount >= min_quantity.amount, Messages::NEED_MIN_QUANTITY );

  // Reference quantity against token contract stats table
  auto sym = _pledge.quantity.symbol.code().raw();
  stats statstable( token_code, sym );
  const auto& st = statstable.get( sym );
  eosio_assert( _pledge.quantity.symbol == st.supply.symbol, Messages::INVALID_SYMBOL );

  // Verify from account has tokens to pledge (either in stake or patreosvault)
  accounts _accounts( PATREOS_VAULT_CODE, pledger.value );
  auto itr = _accounts.find( sym );
  eosio_assert( itr != _accounts.end(), Messages::NO_VAULT_BALANCE );
  eosio_assert( itr->balance.amount >= _pledge.quantity.amount, Messages::NEED_PLEDGE_FUNDS );
  eosio_assert( itr->balance.amount >= 2 * _pledge.quantity.amount, Messages::NEED_LARGER_VAULT_BALANCE );

  eosio_assert( is_pledge_cycle_valid(_pledge.seconds), Messages::INVALID_CYCLE );

  name ram_payer = _self;
  if(_pledge.quantity.amount < min_quantity.amount) {
    ram_payer = pledger; // Pledge whatever you want, but you ain't eating our ram
  }

  pledgetable.emplace( ram_payer, [&]( auto& p ) {
    p = _pledge;
    p.last_pledge = 0; // Not up to action sender
    p.execution_count = 0; // Not up to action sender
  });

  // Send off first pledge with _self authority
  /*
  action(permission_level{ _self, N(eosio.code) },
      N(patreostoken), N(pledge),
      std::make_tuple(from, to, quantity, "<3")).send();
    */
}

void patreosnexus::paid( name from, name to )
{
  require_auth( PATREOS_VAULT_CODE );
  pledges pledgetable( _self, from.value ); // table scoped by pledger
  auto itr = pledgetable.find( to.value );
  eosio_assert( itr != pledgetable.end(), Messages::PLEDGE_DNE );

  pledgetable.modify( itr, same_payer, [&]( auto& p ) {
      p.last_pledge = now();
      p.execution_count++;
  });
}

// Pledges can be terminated by patreosvault if balance overdrawn
void patreosnexus::unpledge( name pledger, name creator )
{
  eosio_assert(has_auth(pledger) || has_auth(PATREOS_VAULT_CODE),
    Messages::NEED_AUTH_FOR_UNPLEDGE);
  pledges pledgetable( _self, pledger.value ); // table scoped by pledger
  auto itr = pledgetable.find( creator.value );
  eosio_assert( itr != pledgetable.end(), Messages::PLEDGE_DNE );

  pledgetable.erase( itr );
}

void patreosnexus::setprofile( name owner, patreosnexus::profile _profile )
{
  require_auth(owner);

  profiles _profiles( _self, _self.value); // table scoped by contract
  auto itr = _profiles.find( owner.value );
  if( itr == _profiles.end() ) {
     _profiles.emplace( owner, [&]( auto& a ){
       a = _profile;
     });
  } else {
     _profiles.modify( itr, same_payer, [&]( auto& a ) {
       a = _profile;
     });
  }
}

void patreosnexus::unsetprofile( name owner )
{
  require_auth(owner);

  profiles _profiles( _self, _self.value ); // table scoped by contract
  auto itr = _profiles.find( owner.value );
  eosio_assert( itr != _profiles.end(), Messages::PLEDGE_DNE );

  _profiles.erase( itr );
}

void patreosnexus::publish( name owner, patreosnexus::publication _publication )
{
  require_auth(owner);
}

EOSIO_DISPATCH( patreosnexus, (follow)(unfollow)(pledge)(unpledge)(publish)(paid)(setprofile)(unsetprofile) )

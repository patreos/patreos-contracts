#include "patreosnexus.hpp"

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
  eosio_assert( is_account( _pledge.creator ), "Creator account does not exist" );

  // Verify pledge doesn't exist already
  pledges pledgetable( _self, pledger.value ); // table scoped by pledger
  auto match = pledgetable.find( _pledge.creator.value );
  eosio_assert( match == pledgetable.end(), "Pledge already exists." );

  // Check pledge quantity
  eosio_assert( is_supported_asset(_pledge.quantity), "We do not support this token currently" );
  eosio_assert( _pledge.quantity.is_valid(), "Invalid quantity" );
  eosio_assert( _pledge.quantity.amount > 0, "Must pledge positive quantity" );

  asset min_quantity;
  name token_code;
  if(_pledge.quantity.symbol == EOS_SYMBOL) {
    min_quantity = min_pledge_eos;
    token_code = EOS_TOKEN_CODE;
  } else if (_pledge.quantity.symbol == PTR_SYMBOL) {
    min_quantity = min_pledge_ptr;
    token_code = PATREOS_TOKEN_CODE;
  } else {
    eosio_assert( false, "Token could not be found" );
  }
  eosio_assert( _pledge.quantity.amount >= min_quantity.amount, "Must pledge at least min quanity" );

  // Reference quantity against token contract stats table
  auto sym = _pledge.quantity.symbol.code().raw();
  stats statstable( token_code, sym );
  const auto& st = statstable.get( sym );
  eosio_assert( _pledge.quantity.symbol == st.supply.symbol, "Symbol precision mismatch" );

  // Verify from account has tokens to pledge (either in stake or patreosvault)
  accounts _accounts( PATREOS_VAULT_CODE, pledger.value );
  auto itr = _accounts.find( sym );
  eosio_assert( itr != _accounts.end(), "You have no balance to pledge" );
  eosio_assert( itr->balance.amount >= _pledge.quantity.amount, "Insufficent funds for pledge amount" );
  eosio_assert( itr->balance.amount >= 2 * _pledge.quantity.amount, "Expected a balance of 2x the pledge" );

  eosio_assert( is_pledge_cycle_valid(_pledge.seconds), "Invalid pledge cycle" );

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
  eosio_assert( itr != pledgetable.end(), "pledge does not exist." );

  pledgetable.modify( itr, same_payer, [&]( auto& p ) {
      p.last_pledge = now();
      p.execution_count++;
  });
}

// Pledges can be terminated by patreosvault if balance overdrawn
void patreosnexus::unpledge( name pledger, name creator )
{
  eosio_assert(has_auth(pledger) || has_auth(PATREOS_VAULT_CODE),
    "Not authorized to unpledge");
  pledges pledgetable( _self, pledger.value ); // table scoped by pledger
  auto itr = pledgetable.find( creator.value );
  eosio_assert( itr != pledgetable.end(), "pledge does not exist." );

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
  eosio_assert( itr != _profiles.end(), "pledge does not exist." );

  _profiles.erase( itr );
}

void patreosnexus::publish( name owner, patreosnexus::publication _publication )
{
  require_auth(owner);
}

EOSIO_DISPATCH( patreosnexus, (follow)(unfollow)(pledge)(unpledge)(publish)(paid)(setprofile)(unsetprofile) )

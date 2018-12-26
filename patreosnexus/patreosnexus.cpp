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

void patreosnexus::pledge( name from, name to )
{
  require_auth(from);
  eosio_assert( is_account( to ), Messages::CREATOR_ACCOUNT_DNE );

  // TODO: if already registered pledge, error

  // TODO: Verify subscription in recurringpay table of patreosnexus

  // TODO: register pledge in table

  // TODO: If stake is sufficient, and pledge executed once, refund executed fee

  // TODO: If stake is sufficient, wave future fees (probably a helper)
}

void patreosnexus::unpledge( name from, name to )
{
  require_auth(from);
  eosio_assert( is_account( to ), Messages::CREATOR_ACCOUNT_DNE );

  // TODO: verify registered pledge in table

  // TODO: Verify no subscription in recurringpay table of patreosnexus

  // TODO: unregister pledge
}

void patreosnexus::setprofile( name owner, patreosnexus::profile _profile )
{
  require_auth(owner);

  // TODO: Validate _profile object

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

  // Store _publication in ram until shared
}

void patreosnexus::blurb( name from, name to, string memo )
{
  eosio_assert( from != to, Messages::CANNOT_BLURB_TO_SELF );
  require_auth( "patreosnexus"_n );
  eosio_assert( is_account( to ), Messages::TO_ACCCOUNT_DNE );
  require_recipient( from );
  require_recipient( to );
}

EOSIO_DISPATCH( patreosnexus, (follow)(unfollow)(pledge)(unpledge)(publish)(setprofile)(unsetprofile)(blurb) )

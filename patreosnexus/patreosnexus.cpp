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

/*
void patreosnexus::checkfees( name from, name to ) {

}
*/

void patreosnexus::pledge( name from, name to )
{
  require_auth(from);
  eosio_assert( is_account( to ), Messages::CREATOR_ACCOUNT_DNE );

  pledges pledges_table(_self, _self.value);
  auto pledges_table_secondary = pledges_table.get_index<"from.to"_n>();
  auto pledge_by_parties_id = combine_ids(from.value, to.value);

  auto pledges_table_secondary_itr = pledges_table_secondary.find( pledge_by_parties_id );

  // TODO: If pledge exists, verify recurringpay exists, if not, let user repledge
  eosio_assert(pledges_table_secondary_itr == pledges_table_secondary.end(), "Pledge already exists!");

  // TODO: Decide on official account
  name patreos_service_account = "patreosnexus"_n;

  agreements agreements_table( "recurringpay"_n, patreos_service_account.value );
  auto agreements_table_secondary = agreements_table.get_index<"from.to"_n>();
  auto agreement_itr = agreements_table_secondary.find( pledge_by_parties_id );
  eosio_assert(agreement_itr != agreements_table_secondary.end(), "Subscription agreement should exist beforehand!");

  pledges_table.emplace( from, [&]( auto& p ){
    p.key = pledges_table.available_primary_key();
    p.from = from;
    p.to = to;
  });

  stakes from_stakes( "patreostoken"_n, from.value );
  auto from_stakes_itr = from_stakes.find( PTR_SYMBOL.code().raw() );
  if(from_stakes_itr != from_stakes.end()) {
    // No fees for sufficient stake
    if(from_stakes_itr->balance >= require_stake_per_pledge) {
      if(agreement_itr->execution_count == 1) {
        print(" Refunding fee, and waving future fees");
        // TODO: refund executed fee (peer review first)

        // TODO: If stake is sufficient, wave future fees (probably a helper)
      }
    }
  }
}

void patreosnexus::unpledge( name from, name to )
{
  require_auth(from);
  eosio_assert( is_account( to ), Messages::CREATOR_ACCOUNT_DNE );

  pledges pledges_table(_self, _self.value);
  auto pledges_table_secondary = pledges_table.get_index<"from.to"_n>();
  auto pledge_by_parties_id = combine_ids(from.value, to.value);

  auto pledges_table_secondary_itr = pledges_table_secondary.find( pledge_by_parties_id );
  eosio_assert(pledges_table_secondary_itr != pledges_table_secondary.end(), "Pledge does not exist!");

  agreements agreements_table( "recurringpay"_n, "patreosnexus"_n.value );
  auto agreements_table_secondary = agreements_table.get_index<"from.to"_n>();
  auto agreement_itr = agreements_table_secondary.find( pledge_by_parties_id );
  eosio_assert(agreement_itr == agreements_table_secondary.end(), "Subscription agreement should first be canceled!");

  pledges_table_secondary.erase( pledges_table_secondary_itr );
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

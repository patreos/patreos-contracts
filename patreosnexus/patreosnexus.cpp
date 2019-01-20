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
// Consider a processessing limit as well
void patreosnexus::checkfees( name from, name to ) {

  // TODO: get number of pledges, get stake, and if stake is sufficient then wave fees
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

  // TODO: Decide on official account
  name patreos_service_account = "patreosnexus"_n;

  agreements agreements_table( "recurringpay"_n, patreos_service_account.value );
  auto agreements_table_secondary = agreements_table.get_index<"from.to"_n>();
  auto agreement_itr = agreements_table_secondary.find( pledge_by_parties_id );


  bool pledge_exists = pledges_table_secondary_itr != pledges_table_secondary.end();
  bool agreement_exists = agreement_itr != agreements_table_secondary.end();

  // Handle all delinquency states between agreements and pledges
  if(agreement_exists) {
    if(pledge_exists) {
      // TODO Update pledge

      // TODO: penalty for past subscription delinquency

      print("(agreement_exists && pledge_exists) == true"); print("\n");
      return;
    }
  } else {
    if(!pledge_exists) {
      print("(!agreement_exists && !pledge_exists) == true"); print("\n");
      eosio_assert(0, "Subscription agreement should exist beforehand!");
    } else {
      pledges_table_secondary.erase(pledges_table_secondary_itr);

      // TODO: penalty for past subscription delinquency

      print("(!agreement_exists && pledge_exists) == true"); print("\n");
    }
  }

  // Here we have an existing agreement and no pledge

  // Get latest publication from creator
  publications publications_table(_self, to.value);
  auto publications_itr = publications_table.begin();
  uint64_t last_publication = 0;
  // Publications table only has at most 4 items, so safe to loop all
  while(publications_itr != publications_table.end()) {
    if(last_publication < publications_itr->key) {
      last_publication = publications_itr->key;
    }
    publications_itr++;
  }

  pledges_table.emplace( from, [&]( auto& p ){
    p.key = pledges_table.available_primary_key();
    p.from = from;
    p.to = to;
    p.last_publication = last_publication;
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

void patreosnexus::setprofile( patreosnexus::profile _profile )
{
  require_auth(_profile.owner);

  // TODO: Validate _profile object

  profiles profiles_table( _self, _self.value); // table scoped by contract
  auto profiles_table_itr = profiles_table.find( _profile.owner.value );
  if( profiles_table_itr == profiles_table.end() ) {
     profiles_table.emplace( _profile.owner, [&]( auto& p ){
       p.owner = _profile.owner;
       p.name = _profile.name;
       p.description = _profile.description;
       p.image_url = _profile.image_url;
       p.banner_url = _profile.banner_url;
       p.last_publication = 0;
     });
  } else {
     profiles_table.modify( profiles_table_itr, same_payer, [&]( auto& p ) {
       p.owner = _profile.owner;
       p.name = _profile.name;
       p.description = _profile.description;
       p.image_url = _profile.image_url;
       p.banner_url = _profile.banner_url;
     });
  }
}

void patreosnexus::unsetprofile( name owner )
{
  require_auth(owner);

  profiles profiles_table( _self, _self.value ); // table scoped by contract
  auto profiles_table_itr = profiles_table.find( owner.value );
  eosio_assert( profiles_table_itr != profiles_table.end(), Messages::PLEDGE_DNE );

  profiles_table.erase( profiles_table_itr );
}

void patreosnexus::publish( name owner, patreosnexus::publication _publication )
{
  require_auth(owner);

  publications publications_table(_self, owner.value);
  uint16_t publications_table_size = 0;
  auto publications_itr = publications_table.begin();
  auto publications_itr_ref = publications_itr;
  bool found_published = false;
  // Publications table only has at most 4 items, so safe to loop all
  while(publications_itr != publications_table.end()) {
    if(!publications_itr->pending && !found_published) {
      publications_itr_ref = publications_itr;
      found_published = true;
    }
    publications_table_size++;
    publications_itr++;
  }

  if(publications_table_size >= 4 && found_published) {
    publications_table.erase(publications_itr_ref);
    publications_table_size--;
  } else {
    eosio_assert( publications_table_size < 4, "Too many pending publications.  Please try later." );
  }

  // Store _publication in ram until shared
  publications_table.emplace( _self, [&]( auto& p ){
    p.key = publications_table.available_primary_key(); // TODO: make sure this is incremented by 1
    p.title = _publication.title;
    p.description = _publication.description;
    p.url = _publication.url;
    p.pending = true;
    p.datetime = now();
  });

}

void patreosnexus::process( name owner , uint16_t process_limit) {
  publications publications_table(_self, owner.value);
  auto publications_itr = publications_table.begin();
  uint16_t processed_count = 0;
  // Publications table only has at most 4 items, so safe to loop all
  while(publications_itr != publications_table.end() && processed_count < process_limit) {
    if(publications_itr->pending) {
      pledges pledges_table(_self, owner.value);
      auto pledges_table_secondary = pledges_table.get_index<"publication"_n>();
      uint64_t key = publications_itr->key - 1;
      auto pledges_itr = pledges_table_secondary.find( key );

      while(pledges_itr != pledges_table_secondary.end() && processed_count < process_limit) {
        if(pledges_itr->last_publication == key) {

          // TODO: Blurb publication
          // blurb( pledges_itr->from, pledges_itr->to, publications_itr->title );

          pledges_table_secondary.modify( pledges_itr, same_payer, [&]( auto& p ) {
            p.last_publication++;
          });
          pledges_itr++;
          processed_count++;
        }
      }

      if(processed_count < process_limit) {
        // Loop exited because no more pledges to publish to, so publication is processed.
        publications_table.modify( publications_itr, same_payer, [&]( auto& p ) {
          p.pending = false;
        });
      }
    }
    publications_itr++;
  }

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

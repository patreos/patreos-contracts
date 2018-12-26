#pragma once

#include <eosiolib/asset.hpp>
#include <eosiolib/eosio.hpp>
#include <string>
#include "../common/patreos.hpp"

namespace eosiosystem {
   class system_contract;
}

using std::string;
using namespace eosio;

class [[eosio::contract("patreosnexus")]] patreosnexus : public contract {
  public:

    struct [[eosio::table]] profile {
      name      owner;
      string    name;
      string    description;

      uint64_t primary_key() const { return owner.value; }
      EOSLIB_SERIALIZE( profile, (owner)(name)(description) )
    };

    struct [[eosio::table]] publication {
      uint64_t    item; // unique
      string      title;
      string      description;
      string      url;

      uint64_t primary_key() const { return item; }
      EOSLIB_SERIALIZE( publication, (item)(title)(description)(url) )
    };

    struct raw_token_profile {
      name contract;
      asset quantity;
    };

    // proto subscription agreement
    struct raw_agreement {
      name from;
      name to;
      raw_token_profile token_profile_amount;
      uint32_t cycle_seconds;
    };

  private:

    // patreostoken table
    struct currency_stats {
      asset    supply;
      asset    max_supply;
      name     issuer;

      uint64_t primary_key() const { return supply.symbol.code().raw(); }
    };

    // patreostoken table
    struct [[eosio::table]] account {
      asset    balance;
      uint64_t primary_key() const { return balance.symbol.code().raw(); }
    };

    //typedef eosio::multi_index<"pledges"_n, pledge_data> pledges; // we pay ram above certain PATR
    typedef eosio::multi_index<"profiles"_n, profile> profiles; // user pays ram
    typedef eosio::multi_index<"publications"_n, publication> publications; // we pay ram, remove after processed

    typedef eosio::multi_index< "usage"_n, account > usage; //we pay ram

    typedef eosio::multi_index< "accounts"_n, account > accounts;
    typedef eosio::multi_index< "stat"_n, currency_stats > stats;

  public:

    using contract::contract;

    [[eosio::action]]
    void follow( name owner, name following );

    [[eosio::action]]
    void unfollow( name owner, name following );

    [[eosio::action]]
    void pledge( name from, name to );

    [[eosio::action]]
    void unpledge( name from, name to );

    [[eosio::action]]
    void setprofile( name owner, profile _profile );

    [[eosio::action]]
    void unsetprofile( name owner );

    [[eosio::action]]
    void publish( name owner, publication _publication );
    
    [[eosio::action]]
    void blurb( name from, name to, string memo );

};

#pragma once

#include <eosiolib/asset.hpp>
#include <eosiolib/eosio.hpp>

#include <string>

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

  private:

    // patreostoken table
    struct [[eosio::table]] account {
      asset    balance;
      uint64_t primary_key() const { return balance.symbol.code().raw(); }
    };

    // patreostoken table
    struct [[eosio::table]] currency_stats {
      asset    supply;
      asset    max_supply;
      name     issuer;

      uint64_t primary_key() const { return supply.symbol.code().raw(); }
    };

    struct [[eosio::table]] pledge_data {
      name        to;
      asset       quantity;
      uint32_t    seconds;
      uint64_t        last_pledge; // now()
      uint16_t    execution_count;

      uint64_t primary_key() const { return to.value; }
    };

    typedef eosio::multi_index<"profiles"_n, profile> profiles; // creator pays ram
    typedef eosio::multi_index<"publications"_n, publication> publications; // creator pays ram (optional)

    // Temp until offchain storage
    typedef eosio::multi_index<"pledges"_n, pledge_data> pledges; // we pay ram above certain PATR

    typedef eosio::multi_index<"stakes"_n, account> stakes;
    typedef eosio::multi_index<"stats"_n, currency_stats> stats;

  public:

    using contract::contract;

    [[eosio::action]]
    void subscribe( name from, name to );

    [[eosio::action]]
    void unsubscribe( name from, name to );

    [[eosio::action]]
    void pledge( name from, name to, uint32_t seconds, asset quantity );

    [[eosio::action]]
    void pledgepaid( name from, name to );

    [[eosio::action]]
    void unpledge( name from, name to );

    [[eosio::action]]
    void depledge( name from, name to );

    [[eosio::action]]
    void setprofile( name owner, profile _profile );

    [[eosio::action]]
    void unsetprofile( name owner );

    [[eosio::action]]
    void publish( name owner, publication _publication );

};

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

    // concatenation of ids example
    uint128_t combine_ids(const uint64_t &x, const uint64_t &y) {
        return (uint128_t{x} << 64) | y;
    };

    struct [[eosio::table]] usage_stats {
      name owner;
      uint64_t pledged;
      uint64_t unpledged;
      uint64_t followed;
      uint64_t unfollowed;
      uint64_t published;
      uint64_t primary_key() const { return owner.value; }
    };

    struct [[eosio::table]] profile {
      name      owner;
      string    name;
      string    description;
      string      image_url;
      string      banner_url;
      uint64_t last_publication;

      uint64_t primary_key() const { return owner.value; }
      EOSLIB_SERIALIZE( profile, (owner)(name)(description)(image_url)(banner_url)(last_publication) )
    };

    struct [[eosio::table]] publication {
      uint64_t  key; // unique
      string  title;
      string  description;
      string  url;
      bool  pending;
      uint64_t datetime;

      uint64_t primary_key() const { return key; }
      EOSLIB_SERIALIZE( publication, (key)(title)(description)(url) )
    };

    struct [[eosio::table]] pledge_item {
      uint64_t    key; // unique
      name      from;
      name      to;
      uint64_t last_publication;

      uint64_t primary_key() const { return key; }
      uint128_t get_pledge_by_parties() const {
        return (uint128_t{from.value} << 64) | to.value;
      }
      uint64_t get_supporter() const {
        return from.value;
      }
      uint64_t get_creator() const {
        return to.value;
      }
      uint64_t get_by_publication() const {
        return last_publication;
      }
      EOSLIB_SERIALIZE( pledge_item, (key)(from)(to)(last_publication) )
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

    // subscription agreement
    struct [[eosio::table]] agreement {
      uint64_t id;
      name from;
      name to;
      raw_token_profile token_profile_amount;
      uint32_t cycle_seconds;
      uint16_t pending_payments;
      uint64_t last_executed;
      uint16_t execution_count;
      asset fee;

      uint64_t primary_key() const { return id; }
      uint128_t get_agreement_by_parties() const {
        return (uint128_t{from.value} << 64) | to.value;
      }
      uint64_t get_payer() const {
        return from.value;
      }
      uint64_t get_receiver() const {
        return to.value;
      }
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

    typedef multi_index<"pledges"_n, pledge_item,
      indexed_by<
        "from.to"_n,
        const_mem_fun <pledge_item, uint128_t, &pledge_item::get_pledge_by_parties>
      >,
      indexed_by<
        "from"_n,
        const_mem_fun <pledge_item, uint64_t, &pledge_item::get_supporter>
      >,
      indexed_by<
        "to"_n,
        const_mem_fun <pledge_item, uint64_t, &pledge_item::get_creator>
      >,
      indexed_by<
        "publication"_n,
        const_mem_fun <pledge_item, uint64_t, &pledge_item::get_by_publication>
      >
    > pledges;

    typedef eosio::multi_index< "usage"_n, usage_stats > usage; //we pay ram

    typedef eosio::multi_index< "accounts"_n, account > accounts;
    typedef eosio::multi_index< "stakes"_n, account > stakes;
    typedef eosio::multi_index< "stat"_n, currency_stats > stats;


    // recurringpay table
    typedef multi_index<"agreements"_n, agreement,
      indexed_by<
        "from.to"_n,
        const_mem_fun <agreement, uint128_t, &agreement::get_agreement_by_parties>
      >,
      indexed_by<
        "from"_n,
        const_mem_fun <agreement, uint64_t, &agreement::get_payer>
      >,
      indexed_by<
        "to"_n,
        const_mem_fun <agreement, uint64_t, &agreement::get_receiver>
      >
    > agreements;

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

    void process( name owner , uint16_t limit );

};

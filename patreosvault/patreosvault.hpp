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

class [[eosio::contract("patreosvault")]] patreosvault : public contract {

  private:

    void sub_balance( name owner, asset value );
    void add_balance( name owner, asset value, name ram_payer );

    // patreostoken table
    struct currency_stats {
      asset    supply;
      asset    max_supply;
      name     issuer;

      uint64_t primary_key() const { return supply.symbol.code().raw(); }
    };

  public:

    patreosvault(name receiver, name code, datastream<const char*> ds) : contract(receiver, code, ds){}

    // patreostoken table
    struct [[eosio::table]] account {
      asset    balance;
      uint64_t primary_key() const { return balance.symbol.code().raw(); }
    };

    // patreostoken table
    struct [[eosio::table]] pledge_data {
      name        to;
      asset       quantity;
      uint32_t    seconds;
      uint64_t    last_pledge; // now()
      uint16_t    execution_count;

      uint64_t primary_key() const { return to.value; }
    };

    struct transfer {
       name     from;
       name     to;
       asset    quantity;
       string   memo;

       EOSLIB_SERIALIZE( transfer, (from)(to)(quantity)(memo) )
    };

    typedef eosio::multi_index<"pledges"_n, pledge_data> pledges; // we pay ram above certain PATR
    typedef eosio::multi_index<"accounts"_n, account> accounts;
    typedef eosio::multi_index<"stat"_n, currency_stats> stats;

    [[eosio::action]]
    void withdraw( name owner, asset quantity );

    [[eosio::action]]
    void process( name processor, name from, name to, asset quantity );

    void transferAction( name self, name code );

    static asset get_balance( name token_contract_account, name owner, symbol_code sym_code )
    {
      accounts accountstable( token_contract_account, owner.value );
      const auto& ac = accountstable.get( sym_code.raw() );
      return ac.balance;
    }
};

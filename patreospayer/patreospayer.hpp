#pragma once

#include <eosiolib/asset.hpp>
#include <eosiolib/eosio.hpp>
#include <string>
#include "../common/patreos.hpp"

namespace eosiosystem {
   class system_contract;
}

using std::string;
using std::vector;
using namespace eosio;

class [[eosio::contract("patreospayer")]] patreospayer : public contract {

  private:

    void sub_balance( name owner, name contract, asset quantity );
    void add_balance( name owner, name contract, asset quantity, name ram_payer );

    // standard stats table
    struct currency_stats {
      asset    supply;
      asset    max_supply;
      name     issuer;

      uint64_t primary_key() const { return supply.symbol.code().raw(); }
    };

  public:

    patreospayer(name receiver, name code, datastream<const char*> ds) : contract(receiver, code, ds){}

    // concatenation of ids example
    uint128_t combine_ids(const uint64_t &x, const uint64_t &y) {
        return (uint128_t{x} << 64) | y;
    }

    // A vault token has the origin contract and asset quantity
    struct [[eosio::table]] vault_token {
      uint64_t id;
      name contract;
      asset quantity;

      uint64_t primary_key() const { return id; }
      uint128_t get_token_by_contract() const {
        return (uint128_t{contract.value} << 64) | quantity.symbol.code().raw();
      }
    };

    // subscription service provider
    struct [[eosio::table]] service {
      name provider;
      vector<vault_token> valid_tokens;

      uint64_t primary_key() const { return provider.value; }
    };

    // subscription agreement
    struct [[eosio::table]] agreement {
      uint64_t id;
      name from;
      name to;
      vault_token vault_token_amount;
      uint64_t cycle;
      uint64_t last_executed;
      asset fee;

      uint64_t primary_key() const { return id; }
      uint128_t get_agreement_by_parties() const {
        return (uint128_t{from.value} << 64) | to.value;
      }
    };


    // patreostoken table
    struct [[eosio::table]] account {
      asset    balance;
      uint64_t primary_key() const { return balance.symbol.code().raw(); }
    };

    struct transfer {
       name     from;
       name     to;
       asset    quantity;
       string   memo;

       EOSLIB_SERIALIZE( transfer, (from)(to)(quantity)(memo) )
    };

    typedef multi_index<"agreements"_n, agreement,
      indexed_by<
        "agreeparties"_n,
        const_mem_fun <agreement, uint128_t, &agreement::get_agreement_by_parties>
      >
    > agreements;

    typedef eosio::multi_index<"services"_n, service> services;

    typedef multi_index<"balances"_n, vault_token,
      indexed_by<
        "tokenbycode"_n,
        const_mem_fun <vault_token, uint128_t, &vault_token::get_token_by_contract>
      >
    > balances;

    typedef eosio::multi_index<"accounts"_n, account> accounts;
    typedef eosio::multi_index<"stat"_n, currency_stats> stats;

    [[eosio::action]]
    void regservice( name provider, vector<vault_token> valid_tokens );

    [[eosio::action]]
    void withdraw( name owner, asset quantity );

    [[eosio::action]]
    void updatefee( name provider, name from, name to, asset fee );

    [[eosio::action]]
    void subscribe( name provider, agreement _agreement );

    void transferAction( name self, name code );

    static asset get_balance( name token_contract_account, name owner, symbol_code sym_code )
    {
      accounts accountstable( token_contract_account, owner.value );
      const auto& ac = accountstable.get( sym_code.raw() );
      return ac.balance;
    }
};

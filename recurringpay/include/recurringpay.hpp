#pragma once

#include <eosiolib/asset.hpp>
#include <eosiolib/eosio.hpp>
#include <string>
#include "common.hpp"

namespace eosiosystem {
   class system_contract;
}

using std::string;
using std::vector;
using namespace eosio;

class [[eosio::contract("recurringpay")]] recurringpay : public contract {

  private:

    void sub_balance( name owner, name contract, asset quantity );
    void add_balance( name owner, name contract, asset quantity, name ram_payer );
    void execute_subscription( name provider, name from, name to,
      name contract, asset quantity, asset fee );

    // standard stats table
    struct currency_stats {
      asset    supply;
      asset    max_supply;
      name     issuer;

      uint64_t primary_key() const { return supply.symbol.code().raw(); }
    };

  public:

    recurringpay(name receiver, name code, datastream<const char*> ds) : contract(receiver, code, ds){}

    // concatenation of ids example
    uint128_t combine_ids(const uint64_t &x, const uint64_t &y) {
        return (uint128_t{x} << 64) | y;
    }

    struct [[eosio::table]] ram_cost {
      name account;
      uint16_t deposits;

      uint64_t primary_key() const { return account.value; }
    };

    struct [[eosio::table]] registration_credit {
      name account;

      uint64_t primary_key() const { return account.value; }
    };

    struct raw_token_profile {
      name contract;
      asset quantity;
    };

    struct [[eosio::table]] token_profile {
      uint64_t id;
      name contract;
      asset quantity;

      uint64_t primary_key() const { return id; }
      uint128_t get_by_code_and_symbol() const {
        return (uint128_t{contract.value} << 64) | quantity.symbol.code().raw();
      }
    };

    // Non-table form
    struct raw_token_service_stat {
      name token_contract;
      //asset min_agreement_quantity;
      asset flat_fee;
      float percentage_fee;
    };

    // Provider tokens have origin contract, and fee descriptions
    struct [[eosio::table]] token_service_stat {
      uint64_t id;
      name token_contract;
      //asset min_agreement_quantity;
      asset flat_fee;
      float percentage_fee;

      uint64_t primary_key() const { return id; }
      uint128_t get_by_code_and_symbol() const {
        return (uint128_t{token_contract.value} << 64) | flat_fee.symbol.code().raw();
      }
    };

    // proto subscription agreement
    struct raw_agreement {
      name from;
      name to;
      raw_token_profile token_profile_amount;
      uint32_t cycle;
    };

    // subscription agreement
    struct [[eosio::table]] agreement {
      uint64_t id;
      name from;
      name to;
      raw_token_profile token_profile_amount;
      uint32_t cycle;
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

    typedef multi_index<"services"_n, token_service_stat,
      indexed_by<
        "code.symbol"_n,
        const_mem_fun <token_service_stat, uint128_t, &token_service_stat::get_by_code_and_symbol>
      >
    > services;

    typedef multi_index<"balances"_n, token_profile,
      indexed_by<
        "code.symbol"_n,
        const_mem_fun <token_profile, uint128_t, &token_profile::get_by_code_and_symbol>
      >
    > balances;

    typedef eosio::multi_index<"ramcosts"_n, ram_cost> ram_costs;
    typedef eosio::multi_index<"regcredit"_n, registration_credit> registration_credits;

    typedef eosio::multi_index<"accounts"_n, account> accounts;
    typedef eosio::multi_index<"stat"_n, currency_stats> stats;


    [[eosio::action]]
    void regservice( name provider, vector<raw_token_service_stat> valid_tokens );

    [[eosio::action]]
    void withdraw( name owner, name contract, asset quantity );

    [[eosio::action]]
    void updatefee( name provider, name from, name to, asset fee );

    [[eosio::action]]
    void subscribe( name provider, raw_agreement agreement );

    [[eosio::action]]
    void unsubscribe( name provider, name from, name to );

    [[eosio::action]]
    void process( name provider, name from, name to );

    void transferAction( name self, name code );

    static asset get_balance( name token_contract_account, name owner, symbol_code sym_code )
    {
      accounts accountstable( token_contract_account, owner.value );
      const auto& ac = accountstable.get( sym_code.raw() );
      return ac.balance;
    }
};

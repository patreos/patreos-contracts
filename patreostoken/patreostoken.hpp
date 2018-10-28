/**
 *  @file
 *  @copyright defined in eos/LICENSE.txt
 */
#pragma once

#include <eosiolib/asset.hpp>
#include <eosiolib/eosio.hpp>
#include <eosiolib/transaction.hpp>
#include <../patreos/commons.hpp>
#include <string>

namespace eosiosystem {
   class system_contract;
}

namespace eosio {

   using std::string;

   class token : public contract {
      public:
         token( account_name self ) : contract(self) {}

         void create( account_name issuer,
                      asset        maximum_supply);

         void issue( account_name to, asset quantity, string memo );

         void transfer( account_name from,
                        account_name to,
                        asset        quantity,
                        string       memo );

         void stake( account_name owner,
                        asset        quantity,
                        string       memo );

         void unstake( account_name owner,
                        asset        quantity,
                        string       memo );

         void unstakeDelay (account_name account,
                        asset        quantity,
                        string       memo);

         void pledge( account_name from,
                        account_name to,
                        asset        quantity,
                        string       memo );

         inline asset get_supply( symbol_name sym )const;

         inline asset get_balance( account_name owner, symbol_name sym )const;

         inline asset get_staked_balance( account_name owner, symbol_name sym )const;
         inline asset get_total_balance( account_name owner, symbol_name sym )const;

      private:
         struct account {
            asset    balance;

            uint64_t primary_key()const { return balance.symbol.name(); }
         };

         struct currency_stats {
            asset          supply;
            asset          max_supply;
            account_name   issuer;

            uint64_t primary_key()const { return supply.symbol.name(); }
         };

         typedef eosio::multi_index<N(accounts), account> accounts;
         typedef eosio::multi_index<N(liquidstake), account> liquidstake; // Table for stake for pledging
         typedef eosio::multi_index<N(lockedstake), account> lockedstake; // Table for stake for Patreos resources
         typedef eosio::multi_index<N(stat), currency_stats> stats;

         void sub_balance( account_name owner, asset value );
         void add_balance( account_name owner, asset value, account_name ram_payer );
         void add_balance_to_existing_user( account_name owner, asset value );
         void sub_staked_balance( account_name owner, asset value );
         void add_staked_balance( account_name owner, asset value );

      public:
         struct transfer_args {
            account_name  from;
            account_name  to;
            asset         quantity;
            string        memo;
         };
   };

   asset token::get_supply( symbol_name sym )const
   {
      stats statstable( _self, sym );
      const auto& st = statstable.get( sym );
      return st.supply;
   }

   asset token::get_balance( account_name owner, symbol_name sym )const
   {
      accounts accountstable( _self, owner );
      const auto& ac = accountstable.get( sym );
      return ac.balance;
   }

   asset token::get_staked_balance( account_name owner, symbol_name sym )const
   {
      liquidstake stakedaccountstable( _self, owner );
      const auto& sac = stakedaccountstable.get( sym );
      return sac.balance;
   }

   asset token::get_total_balance( account_name owner, symbol_name sym )const
   {
      return token::get_balance(owner, sym) + token::get_staked_balance(owner, sym);
   }

} /// namespace eosio

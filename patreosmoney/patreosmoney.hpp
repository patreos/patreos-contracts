#pragma once

#include <eosiolib/asset.hpp>
#include <eosiolib/eosio.hpp>
#include <string>
#include <unordered_map>

namespace eosiosystem {
   class system_contract;
}

using std::string;
using namespace eosio;

class [[eosio::contract("patreosmoney")]] patreosmoney : public contract {
  public:

    // concatenation of ids example
    uint128_t combine_ids(const uint64_t &x, const uint64_t &y) {
        return (uint128_t{x} << 64) | y;
    };

    asset get_round_rewards( name user, uint64_t round_id );

    struct [[eosio::table]] account {
      asset    balance;
      uint64_t primary_key()const { return balance.symbol.code().raw(); }
    };

    struct [[eosio::table]] patrglobal {
      uint64_t id;
      asset    staked;
      asset    unstaked;
      uint64_t primary_key()const { return id; }
    };

  private:

    struct [[eosio::table]] round_id {
      uint64_t id;

      uint64_t primary_key() const { return id; }
    };

    struct [[eosio::table]] user {
      name account;

      uint64_t primary_key() const { return account.value; }
    };

    struct [[eosio::table]] delta {
      name account;
      asset stake;

      uint64_t primary_key() const { return account.value; }
    };

    struct [[eosio::table]] global {
      uint64_t id;
      uint64_t active_round_id;
      uint64_t last_deleted_round_id;
      uint64_t active_round_duration;
      uint64_t max_round_lifespan;
      asset max_round_payout;
      bool active;

      uint64_t primary_key() const { return id; }
    };

    struct [[eosio::table]] reward_round {
      uint64_t id;
      bool active;
      uint64_t start_time;
      asset staked;
      asset round_payout;

      uint64_t primary_key() const { return id; }
    };

    typedef eosio::multi_index< "globals"_n, global > globals;
    typedef eosio::multi_index< "rounds"_n, reward_round > rounds;

    typedef eosio::multi_index< "accounts"_n, account > accounts;
    typedef eosio::multi_index< "patrglobals"_n, patrglobal > patrglobals;
    typedef eosio::multi_index< "stakes"_n, account > stakes;

    typedef eosio::multi_index< "expired"_n, round_id > expired; // Scoped by _self
    typedef eosio::multi_index< "derewarded"_n, user > derewarded; // Scoped by _self
    typedef eosio::multi_index< "disqualified"_n, user > disqualified; // Scoped by round id
    typedef eosio::multi_index< "deltas"_n, delta > deltas; // Scoped by round id


  public:

    using contract::contract;

    [[eosio::action]]
    void claim( name user );

    [[eosio::action]]
    void newround( name initialier );

    [[eosio::action]]
    void cleanup( name initialier );

    [[eosio::action]]
    void slashreward( name user );

    [[eosio::action]]
    void reportdelta( name user, asset old_stake );

    [[eosio::action]]
    void setglobal(
      uint64_t active_round_duration,
      uint64_t max_round_lifespan,
      asset max_round_payout
    );
};

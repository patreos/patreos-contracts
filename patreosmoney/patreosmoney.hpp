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

  private:

    struct global {
      uint64_t id;
      uint64_t active_round;
      uint64_t round_duration;
      uint64_t max_round_lifespan;
      bool active;

      uint64_t primary_key() const { return id; }
    };

    struct reward_round {
      uint64_t id;
      bool active;
      uint64_t start_time;
      asset staked;
      std::unordered_map<uint64_t, uint64_t> blacklist;
      std::unordered_map<uint64_t, asset> history;

      uint64_t primary_key() const { return id; }
    };

    typedef eosio::multi_index< "rounds"_n, reward_round > rewards;


  public:

    using contract::contract;

    [[eosio::action]]
    void claim( name owner );

    [[eosio::action]]
    void staked( name owner, asset staked );

    [[eosio::action]]
    void unstaked( name owner, asset unstaked );

    [[eosio::action]]
    void newround( asset current_stake );

};

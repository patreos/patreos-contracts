#include "patreosmoney.hpp"

using namespace eosio;

void patreosmoney::claim( name owner )
{
  require_auth(owner);

  // verify that global->active is true
  // check if owner has claim in oldest round
  // ... this means checking that owner not in blacklist, if not in blacklist
  // ... then if reward_round->history[owner.value] exists, reward based on reward_round->history[owner.value] / reward_round->staked
  // ... else reward patreostoken staked balance / reward_round->staked, then add to blacklist
  // If patreosmoney has insufficient funds, reward remaining balance.  Inflation has ended. Set global->active to false,
  // ... and erase all rounds

  // repeat the claim for the next round, until active is reached

  // if oldest round is older than max lifetime, then remove
}

void patreosmoney::newround( asset current_stake )
{

  // verify that patreosmoney has PATR and global->active is true
  // check active round, and if (now - active_reward_round->start_time) > global->round_duration,
  // ... set current round to inactive, create new round, and set global->active_round to id

  // if oldest round is older than global->max_round_lifespan, then remove

}

void staked( name owner, asset staked )
{
  require_auth("patreostoken"_n);
}

void unstaked( name owner, asset unstaked )
{
  require_auth("patreostoken"_n);
}

EOSIO_DISPATCH( patreosmoney, (claim)(newround) )

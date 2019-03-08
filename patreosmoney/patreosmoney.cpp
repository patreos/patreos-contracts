#include "patreosmoney.hpp"

using namespace eosio;

void patreosmoney::setglobal(
  uint64_t active_round_duration,
  uint64_t max_round_lifespan,
  asset max_round_payout
)
{
  require_auth("patreosvault"_n);

  globals globals_table( _self, _self.value );
  const auto& globals_table_itr = globals_table.begin();
  if( globals_table_itr == globals_table.end() ) {
     globals_table.emplace( _self, [&]( auto& g ){
       g.id = 0;
       g.active_round_id = 1;
       g.last_deleted_round_id = 0;
       g.active_contract_duration = 0;
       g.active_round_duration = active_round_duration;
       g.max_round_lifespan = max_round_lifespan;
       g.max_round_payout = max_round_payout;
       g.date_activated = now();
       g.can_withdraw = false;
       g.active = false;
       g.required_votes = 21;
       g.votes = 0;
     });

     patrglobals patreostoken_globals_table( "patreostoken"_n, "patreostoken"_n.value );
     auto patreostoken_globals_table_itr = patreostoken_globals_table.get( 0, "Could not find patreostoken globals" );
     asset current_total_staked = patreostoken_globals_table_itr.staked - patreostoken_globals_table_itr.unstaked;
     eosio_assert( current_total_staked.amount > 0, "Invalid current total stake");

     rounds rounds_table( _self, _self.value );
     rounds_table.emplace( _self, [&]( auto& r ){
       r.id = 1;
       r.active = true;
       r.start_time = now();
       r.staked = current_total_staked;
       r.round_payout = max_round_payout;
     });
  } else {
     globals_table.modify( globals_table_itr, _self, [&]( auto& g ) {
       g.active_contract_duration = 0; // TODO: needs to be a variable
       g.active_round_duration = active_round_duration;
       g.max_round_lifespan = max_round_lifespan;
       g.max_round_payout = max_round_payout;
       g.can_withdraw = false; // TODO: needs to be a variable
     });
  }
}

void patreosmoney::vote(name user, bool launch, asset max_round_payout)
{
  require_auth(user);

  globals globals_table( _self, _self.value );
  const auto& globals_table_itr = globals_table.begin();
  eosio_assert( globals_table_itr != globals_table.end(), "Global object not found!");
  eosio_assert( globals_table_itr->votes < globals_table_itr->required_votes, "Voting has completed!");

  votes votes_table( _self, _self.value );
  auto votes_table_itr = votes_table.find( user.value );
  if( votes_table_itr == votes_table.end() ) {
     votes_table.emplace( _self, [&]( auto& p ){
       p.account = user;
       p.launch = launch;
       p.max_round_payout = max_round_payout;
     });
     globals_table.modify( globals_table_itr, same_payer, [&]( auto& g ) {
       g.votes++;
     });
  } else {
     votes_table.modify( votes_table_itr, same_payer, [&]( auto& p ) {
       p.launch = launch;
       p.max_round_payout = max_round_payout;
     });
  }

  // TODO: if voter table has 10%, then activate
  if(globals_table_itr->votes >= globals_table_itr->required_votes) {
    globals_table.modify( globals_table_itr, same_payer, [&]( auto& g ) {
      g.active = true;
    });
  }
}

void patreosmoney::cleanvotes()
{
  require_auth("patreosvault"_n);

  votes votes_table( _self, _self.value );
  auto votes_table_itr = votes_table.begin();
  while(votes_table_itr != votes_table.end()) {
    votes_table.erase(votes_table_itr++);
  }
  globals globals_table( _self, _self.value );
  const auto& globals_table_itr = globals_table.begin();
  eosio_assert( globals_table_itr != globals_table.end(), "Global object not found!");
  globals_table.modify( globals_table_itr, same_payer, [&]( auto& g ) {
    g.votes = 0;
  });
}

void patreosmoney::deactivate()
{
  require_auth("patreosvault"_n);

  globals globals_table( _self, _self.value );
  const auto& globals_table_itr = globals_table.begin();
  eosio_assert( globals_table_itr != globals_table.end(), "Global object not found!");
  globals_table.modify( globals_table_itr, _self, [&]( auto& g ) {
    g.active = false;
  });
}

asset patreosmoney::_get_round_rewards( name user, uint64_t round_id )
{
  rounds rounds_table( _self, _self.value );
  const auto& rounds_table_itr = rounds_table.get( round_id, "Active round not found" );

  // Checking that user not disqualified for round
  disqualified disqualified_table( _self, round_id );
  const auto& disqualified_table_itr = disqualified_table.find( user.value );
  if(disqualified_table_itr != disqualified_table.end()) {
    return 0 * rounds_table_itr.round_payout;
  }

  // Checking for original round stake
  deltas delta_table( _self, round_id );
  const auto& delta_table_itr = delta_table.find( user.value );
  if(delta_table_itr != delta_table.end()) {
    disqualified_table.emplace( _self, [&]( auto& d ){
      d.account = user;
    });
    return rounds_table_itr.round_payout * (delta_table_itr->stake.amount / rounds_table_itr.staked.amount);
  }

  // ... else reward patreostoken staked balance / reward_round->staked, then add to blacklist
  stakes stakes_table( "patreostoken"_n, user.value );
  const auto& stakes_table_itr = stakes_table.get( rounds_table_itr.round_payout.symbol.code().raw(), "No stake balance object" );
  disqualified_table.emplace( _self, [&]( auto& d ){
    d.account = user;
  });
  return rounds_table_itr.round_payout * (stakes_table_itr.balance.amount / rounds_table_itr.staked.amount);
}

void patreosmoney::withdraw( name user )
{
  require_auth(user);

  globals globals_table( _self, _self.value );
  const auto& globals_table_itr = globals_table.get( 0, "No globals object found" );
  eosio_assert( globals_table_itr.can_withdraw, "Withdraw is currently not available." );
}

void patreosmoney::claim( name user )
{
  require_auth(user);

  // verify that global->active is true
  globals globals_table( _self, _self.value );
  const auto& globals_table_itr = globals_table.get( 0, "No globals object found" );
  eosio_assert( globals_table_itr.active, "Inflation rewards are not active" );

  asset available_claim = 0 * globals_table_itr.max_round_payout;
  uint64_t claim_round = globals_table_itr.last_deleted_round_id;
  while(claim_round < globals_table_itr.active_round_id) { // No more than 5 loops
    available_claim += _get_round_rewards(user, ++claim_round);
  }
  print("Claiming: ", available_claim); print("\n");

  // If patreosmoney has insufficient funds, reward remaining balance.  Inflation has ended. Set global->active to false,
  // ... and erase all rounds
  accounts accounts_table( "patreostoken"_n, _self.value );
  const auto& patr_symbol = eosio::symbol("PATR", 4);
  const auto& accounts_table_itr = accounts_table.get( patr_symbol.code().raw(), "No balance object found" );
  if(accounts_table_itr.balance < available_claim) {
    available_claim = accounts_table_itr.balance;
    globals_table.modify( globals_table_itr, _self, [&]( auto& g ) {
      g.active = false;
    });
  }

  // Add to claim balance
  if(available_claim.amount > 0) {
    payouts payouts_table( _self, _self.value );
    auto payouts_table_itr = payouts_table.find( user.value );
    if( payouts_table_itr == payouts_table.end() ) {
       payouts_table.emplace( _self, [&]( auto& p ){
         p.account = user;
         p.reward = available_claim;
       });
    } else {
       payouts_table.modify( payouts_table_itr, same_payer, [&]( auto& p ) {
         p.reward += available_claim;
       });
    }
  }

  // if oldest round is older than max lifetime, then remove
  rounds rounds_table( _self, _self.value );
  const auto& oldest_rounds_table_itr = rounds_table.begin();
  eosio_assert( oldest_rounds_table_itr != rounds_table.end(), "No rounds found" );
  if( now() - oldest_rounds_table_itr->start_time >= globals_table_itr.max_round_lifespan) {
    expired expired_table( _self, _self.value );
    expired_table.emplace( _self, [&]( auto& e ){
      e.id = oldest_rounds_table_itr->id;
    });
    rounds_table.erase( oldest_rounds_table_itr );
  }
}

void patreosmoney::newround( name initialier )
{
  require_auth(initialier);

  accounts accounts_table( "patreostoken"_n, _self.value );

  const auto& patr_symbol = eosio::symbol("PATR", 4);
  const auto& accounts_table_itr = accounts_table.get( patr_symbol.code().raw(), "No balance object found" );

  globals globals_table( _self, _self.value );
  const auto& globals_table_itr = globals_table.get( 0, "No globals object found" );

  // verify that patreosmoney has PATR and global->active is true
  eosio_assert( globals_table_itr.active, "Inflation rewards are no longer active" );
  if(globals_table_itr.active_contract_duration > 0) {
    eosio_assert( now() - globals_table_itr.date_activated < globals_table_itr.active_contract_duration, "Inflation rewards are no longer active.  Contract expired." );
  }
  eosio_assert( accounts_table_itr.balance.amount > 0, "Inflation balance is empty.  No more payouts.");

  rounds rounds_table( _self, _self.value );
  const auto& rounds_table_itr = rounds_table.get( globals_table_itr.active_round_id, "Active round not found" );
  uint64_t active_round_id = rounds_table_itr.id;

  // check active round, and if (now - active_reward_round->start_time) > global->active_round_duration,
  // ... set current round to inactive, create new round, and set global->active_round to id
  if(now() - rounds_table_itr.start_time >= globals_table_itr.active_round_duration) {
    rounds_table.modify( rounds_table_itr, same_payer, [&]( auto& r ) {
      r.active = false;
    });

    patrglobals patreostoken_globals_table( "patreostoken"_n, "patreostoken"_n.value );
    auto patreostoken_globals_table_itr = patreostoken_globals_table.get( 0, "Could not find patreostoken globals" );
    asset current_total_staked = patreostoken_globals_table_itr.staked - patreostoken_globals_table_itr.unstaked;
    eosio_assert( current_total_staked.amount > 0, "Invalid current total stake");

    // Weekly inflation is 764,122 PATR
    const eosio::asset round_payout = eosio::asset(
      (uint64_t) ( std::pow(10, 4) * 2 * 764122 ),
      patr_symbol
    );

    rounds_table.emplace( _self, [&]( auto& r ){
      r.id = ++active_round_id;
      r.active = true;
      r.start_time = now();
      r.staked = current_total_staked;
      r.round_payout = (accounts_table_itr.balance >= globals_table_itr.max_round_payout) ? globals_table_itr.max_round_payout : accounts_table_itr.balance;
    });

    globals_table.modify( globals_table_itr, same_payer, [&]( auto& g ) {
      g.active_round_id = active_round_id;
    });

    // TODO: reward initializer
  }

  // if oldest round is older than global->max_round_lifespan, then remove
  const auto& oldest_rounds_table_itr = rounds_table.begin();
  eosio_assert( oldest_rounds_table_itr != rounds_table.end(), "No rounds found" );

  if( now() - oldest_rounds_table_itr->start_time >= globals_table_itr.max_round_lifespan) {
    expired expired_table( _self, _self.value );
    expired_table.emplace( _self, [&]( auto& e ){
      e.id = oldest_rounds_table_itr->id;
    });
    rounds_table.erase( oldest_rounds_table_itr );

    globals_table.modify( globals_table_itr, same_payer, [&]( auto& g ) {
      g.last_deleted_round_id = oldest_rounds_table_itr->id;
    });
  }
}

void patreosmoney::slashreward(name user)
{
  require_auth("patreostoken"_n);

  globals globals_table( _self, _self.value );
  const auto& globals_table_itr = globals_table.get( 0, "No globals object found" );

  disqualified disqualified_table( _self, globals_table_itr.active_round_id );
  const auto& disqualified_table_itr = disqualified_table.find( user.value );

  if( disqualified_table_itr == disqualified_table.end() ) {
    disqualified_table.emplace( _self, [&]( auto& d ){
      d.account = user;
    });
  }
}


void patreosmoney::reportdelta(name user, asset old_stake)
{
  require_auth("patreostoken"_n);

  globals globals_table( _self, _self.value );
  const auto& globals_table_itr = globals_table.get( 0, "No globals object found" );

  deltas delta_table( _self, globals_table_itr.active_round_id );
  const auto& delta_table_itr = delta_table.find( user.value );

  if( delta_table_itr == delta_table.end() ) {
    delta_table.emplace( _self, [&]( auto& d ){
      d.account = user;
      d.stake = old_stake;
    });
  }
}


void patreosmoney::cleanup( name initialier )
{
  require_auth(initialier);

  expired expired_table( _self, _self.value );
  const auto& expired_table_itr = expired_table.begin();
  eosio_assert( expired_table_itr != expired_table.end(), "No cleanup items found" );

  uint16_t erase_limit = 50;
  uint16_t deltas_table_erased = 0;
  uint16_t disqualified_table_erased = 0;

  deltas deltas_table( _self, expired_table_itr->id );
  auto deltas_table_itr = deltas_table.begin();
  while(deltas_table_itr != deltas_table.end()) {
    if(deltas_table_erased == erase_limit) break;
    deltas_table.erase(deltas_table_itr++);
    deltas_table_erased++;
  }

  disqualified disqualified_table( _self, expired_table_itr->id );
  auto disqualified_table_itr = disqualified_table.begin();
  while(disqualified_table_itr != disqualified_table.end()) {
    if(disqualified_table_erased == erase_limit) break;
    disqualified_table.erase(disqualified_table_itr++);
    disqualified_table_erased++;
  }

  if(deltas_table_erased < erase_limit && disqualified_table_erased < erase_limit) {
    expired_table.erase(expired_table_itr);
  }

  globals globals_table( _self, _self.value );
  const auto& globals_table_itr = globals_table.get( 0, "No globals object found" );

  rounds rounds_table( _self, _self.value );
  const auto& oldest_rounds_table_itr = rounds_table.begin();
  eosio_assert( oldest_rounds_table_itr != rounds_table.end(), "No rounds found" );
  if( now() - oldest_rounds_table_itr->start_time >= globals_table_itr.max_round_lifespan) {
    expired expired_table( _self, _self.value );
    expired_table.emplace( _self, [&]( auto& e ){
      e.id = oldest_rounds_table_itr->id;
    });
    rounds_table.erase( oldest_rounds_table_itr );
  }

  // TODO: reward initializer
}

EOSIO_DISPATCH( patreosmoney, (setglobal)(vote)(cleanvotes)(newround)(claim)(withdraw)(cleanup)(slashreward)(reportdelta)(deactivate) )

#include "patreosnexus.hpp"

/// @abi action
void patreosnexus::subscribe(account_name from, account_name to)
{
    require_auth(from);
}

/// @abi action
void patreosnexus::unsubscribe(account_name from, account_name to)
{
    require_auth(from);
}

/// @abi action
void patreosnexus::pledge(account_name from, account_name to, uint16_t days, asset quantity)
{
    require_auth(from);
    pledges from_pledges( _self, from ); // table scoped by pledger
    auto match = from_pledges.find( to );
    eosio_assert( match == from_pledges.end(), "pledge already exists." );

    /* //TODO: get patreospayer stats
    auto sym = quantity.symbol.name();
    stats statstable( N(patreostoken), sym );
    const auto& st = statstable.get( sym );
    eosio_assert( quantity.is_valid(), "invalid quantity" );
    eosio_assert( quantity.amount > 0, "must transfer positive quantity" );
    eosio_assert( quantity.symbol == st.supply.symbol, "symbol precision mismatch" );
    */

    from_pledges.emplace( _self, [&]( auto& p ) { // We pay the ram
      p.to = to;
      p.days = days;
      p.quantity = quantity;
      p.last_pledge = now();
      p.execution_count = 0;
    });

}

/// @abi action
void patreosnexus::unpledge(account_name from, account_name to)
{
    require_auth(from);
    pledges from_pledges( _self, from ); // table scoped by pledger
    auto itr = from_pledges.find( to );
    eosio_assert( itr != from_pledges.end(), "pledge does not exist." );

    from_pledges.erase( itr );
}

// Managed only by patreospayer code
void patreosnexus::pledge_paid(account_name from, account_name to)
{
    require_auth( N(patreospayer) );
    pledges from_pledges( _self, from ); // table scoped by pledger
    auto match = from_pledges.find( to );
    eosio_assert( match != from_pledges.end(), "pledge does not exist." );


    from_pledges.modify( match, 0, [&]( auto& p ) {
        p.last_pledge = now();
        p.execution_count++;
    });

}

/// @abi action
void patreosnexus::setprofile(account_name owner, profile _profile)
{
    // owner pays ram
    require_auth(owner);
}

/// @abi action
void patreosnexus::unsetprofile(account_name owner)
{
    require_auth(owner);
}

/// @abi action
void patreosnexus::publish(account_name owner, publication _publication)
{
    // _self pays ram
    require_auth(owner);
}

EOSIO_ABI( patreosnexus, (subscribe)(unsubscribe)(pledge)(unpledge)(setprofile)(unsetprofile)(publish) )

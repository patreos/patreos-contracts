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

    // Min pledge before we pay
    asset min_quantity = asset(1, symbol_type(S(4, PTR)));

    // Verify pledge doesn't exist already
    pledges from_pledges( _self, from ); // table scoped by pledger
    auto match = from_pledges.find( to );
    eosio_assert( match == from_pledges.end(), "pledge already exists." );

    // Verify pledge quantity
    auto sym = quantity.symbol.name();
    stats statstable( N(patreostoken), sym );
    const auto& st = statstable.get( sym );
    eosio_assert( quantity.is_valid(), "invalid quantity" );
    eosio_assert( quantity.amount > 0, "must transfer positive quantity" );
    eosio_assert( quantity.symbol == st.supply.symbol, "symbol precision mismatch" );

    // Verify from account has tokens to pledge
    liquidstake liquidtable( N(patreostoken), from);
    auto lt = liquidtable.find( quantity.symbol.name() );
    eosio_assert( lt != liquidtable.end(), "liquid stake balance not found." );
    eosio_assert( lt->balance.amount >= quantity.amount, "insufficent liquid stake for pledge amount" );
    eosio_assert( lt->balance.amount >= 2 * quantity.amount, "expected a liquid stake of 2x the pledge amount" );


    if(quantity.amount >= min_quantity.amount) {
      // We pay ram
      from_pledges.emplace( _self, [&]( auto& p ) {
        p.to = to;
        p.days = days;
        p.quantity = quantity;
        p.last_pledge = now();
        p.execution_count = 1;
      });
    } else {
      // They pay ram
      from_pledges.emplace( from, [&]( auto& p ) {
        p.to = to;
        p.days = days;
        p.quantity = quantity;
        p.last_pledge = now();
        p.execution_count = 1;
      });
    }

    // Send off first pledge with _self authority
    
    /*
    action(permission_level{ _self, N(eosio.code) },
        N(patreostoken), N(pledge),
        std::make_tuple(from, to, quantity, "<3")).send();
      */
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

/// @abi action
void patreosnexus::process(account_name from, account_name to, asset quantity)
{
    // This is a public action
    pledges from_pledges( _self, from ); // table scoped by pledger
    auto match = from_pledges.find( to );
    eosio_assert( match != from_pledges.end(), "pledge does not exist." );

    // Verify pledge quantity
    auto sym = quantity.symbol.name();
    stats statstable( N(patreostoken), sym );
    const auto& st = statstable.get( sym );
    eosio_assert( quantity.is_valid(), "invalid quantity" );
    eosio_assert( quantity.amount > 0, "must transfer positive quantity" );
    eosio_assert( quantity.symbol == st.supply.symbol, "symbol precision mismatch" );

    double milliseconds = double(now() - match->last_pledge);
    int days = (int) ( milliseconds / (1000 * 60 * 60 * 24) );
    eosio_assert( days >= match->days, "pledge subscription not due" );

    // Verify from account has funds

    // Pay pledge amount
    action(permission_level{ _self, N(eosio.code) },
        N(patreostoken), N(pledge),
        std::make_tuple(from, to, quantity, "<3")).send();

    // Increment execution_count and update last_pledge
    SEND_INLINE_ACTION( *this, pledge_paid, { _self, N(eosio.code) }, { from, to } );

}

// Managed only by patreospayer code
void patreosnexus::pledge_paid(account_name from, account_name to)
{
    require_auth( _self );
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

EOSIO_ABI( patreosnexus, (subscribe)(unsubscribe)(pledge)(unpledge)(setprofile)(unsetprofile)(publish)(process) )

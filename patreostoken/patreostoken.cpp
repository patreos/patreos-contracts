#include "patreostoken.hpp"
#include "../common/messages.hpp"

using namespace eosio;

void patreostoken::create( name issuer, asset maximum_supply )
{
    require_auth( _self );

    auto sym = maximum_supply.symbol;
    eosio_assert( sym.is_valid(), Messages::INVALID_SYMBOL_NAME );
    eosio_assert( maximum_supply.is_valid(), Messages::INVALID_SUPPLY);
    eosio_assert( maximum_supply.amount > 0, Messages::NEED_POSITIVE_MAX_SUPPLY);

    stats statstable( _self, sym.code().raw() );
    auto existing = statstable.find( sym.code().raw() );
    eosio_assert( existing == statstable.end(), Messages::SYMBOL_EXISTS );

    statstable.emplace( _self, [&]( auto& s ) {
       s.supply.symbol = maximum_supply.symbol;
       s.max_supply    = maximum_supply;
       s.issuer        = issuer;
    });
}

void patreostoken::issue( name to, asset quantity, string memo )
{
    auto sym = quantity.symbol;
    eosio_assert( sym.is_valid(), Messages::INVALID_SYMBOL_NAME );
    eosio_assert( memo.size() <= 256, Messages::MEMO_TOO_LONG );

    stats statstable( _self, sym.code().raw() );
    auto existing = statstable.find( sym.code().raw() );
    eosio_assert( existing != statstable.end(), Messages::TOKEN_DNE_YET );
    const auto& st = *existing;

    require_auth( st.issuer );
    eosio_assert( quantity.is_valid(), Messages::INVALID_QUANTITY );
    eosio_assert( quantity.amount > 0, Messages::NEED_POSITIVE_ISSUE_QUANTITY );

    eosio_assert( quantity.symbol == st.supply.symbol, Messages::INVALID_SYMBOL );
    eosio_assert( quantity.amount <= st.max_supply.amount - st.supply.amount, Messages::EXCEEDS_SUPPLY);

    statstable.modify( st, same_payer, [&]( auto& s ) {
       s.supply += quantity;
    });

    add_balance( st.issuer, quantity, st.issuer );

    if( to != st.issuer ) {
      SEND_INLINE_ACTION( *this, transfer, { {st.issuer, "active"_n} },
                          { st.issuer, to, quantity, memo }
      );
    }
}

void patreostoken::retire( asset quantity, string memo )
{
    auto sym = quantity.symbol;
    eosio_assert( sym.is_valid(), Messages::INVALID_SYMBOL_NAME );
    eosio_assert( memo.size() <= 256, Messages::MEMO_TOO_LONG );

    stats statstable( _self, sym.code().raw() );
    auto existing = statstable.find( sym.code().raw() );
    eosio_assert( existing != statstable.end(), Messages::TOKEN_DNE );
    const auto& st = *existing;

    require_auth( st.issuer );
    eosio_assert( quantity.is_valid(), Messages::INVALID_QUANTITY );
    eosio_assert( quantity.amount > 0, Messages::NEED_POSITIVE_RETIRE_QUANTITY );

    eosio_assert( quantity.symbol == st.supply.symbol, Messages::INVALID_SYMBOL );

    statstable.modify( st, same_payer, [&]( auto& s ) {
       s.supply -= quantity;
    });

    sub_balance( st.issuer, quantity );
}

void patreostoken::transfer( name    from,
                      name    to,
                      asset   quantity,
                      string  memo )
{
    eosio_assert( from != to, Messages::CANNOT_TRANSFER_TO_SELF );
    require_auth( from );
    eosio_assert( is_account( to ), Messages::ACCOUNT_DNE);
    auto sym = quantity.symbol.code();
    stats statstable( _self, sym.raw() );
    const auto& st = statstable.get( sym.raw() );

    require_recipient( from );
    require_recipient( to );

    eosio_assert( quantity.is_valid(), Messages::INVALID_QUANTITY );
    eosio_assert( quantity.amount > 0, Messages::NEED_POSITIVE_TRANSFER_QUANTITY );
    eosio_assert( quantity.symbol == st.supply.symbol, Messages::INVALID_SYMBOL );
    eosio_assert( memo.size() <= 256, Messages::MEMO_TOO_LONG );

    auto payer = has_auth( to ) ? to : from;

    sub_balance( from, quantity );
    add_balance( to, quantity, payer );
}

void patreostoken::sub_balance( name owner, asset value ) {
   accounts from_acnts( _self, owner.value );

   const auto& from = from_acnts.get( value.symbol.code().raw(), Messages::NO_BALANCE_OBJECT );
   eosio_assert( from.balance.amount >= value.amount, Messages::OVERDRAWN_BALANCE );

   from_acnts.modify( from, owner, [&]( auto& a ) {
     a.balance -= value;
   });
}

void patreostoken::sub_stake( name owner, asset value ) {
   stakes from_stakes( _self, owner.value );

   const auto& from = from_stakes.get( value.symbol.code().raw(), Messages::NO_BALANCE_OBJECT );
   eosio_assert( from.balance.amount >= value.amount, Messages::OVERDRAWN_BALANCE );

   from_stakes.modify( from, owner, [&]( auto& a ) {
     a.balance -= value;
   });
}

void patreostoken::add_balance( name owner, asset value, name ram_payer )
{
   accounts to_acnts( _self, owner.value );
   auto to = to_acnts.find( value.symbol.code().raw() );
   if( to == to_acnts.end() ) {
      to_acnts.emplace( ram_payer, [&]( auto& a ){
        a.balance = value;
      });
   } else {
      to_acnts.modify( to, same_payer, [&]( auto& a ) {
        a.balance += value;
      });
   }
}

void patreostoken::add_stake( name owner, asset value, name ram_payer )
{
   stakes to_stakes( _self, owner.value );
   auto to = to_stakes.find( value.symbol.code().raw() );
   if( to == to_stakes.end() ) {
      to_stakes.emplace( ram_payer, [&]( auto& a ){
        a.balance = value;
      });
   } else {
      to_stakes.modify( to, same_payer, [&]( auto& a ) {
        a.balance += value;
      });
   }
}

void patreostoken::stake( name account, asset quantity )
{
    require_auth( account );
    auto sym = quantity.symbol;
    stats statstable( _self, sym.code().raw() );
    const auto& st = statstable.get( sym.code().raw(), Messages::TOKEN_DNE_YET );

    eosio_assert( quantity.is_valid(), "invalid quantity" );
    eosio_assert( quantity.amount > 0, "must transfer positive quantity" );
    eosio_assert( quantity.symbol == st.supply.symbol, "symbol precision mismatch" );

    sub_balance( account, quantity );
    add_stake( account, quantity, account );
}

// Will have a cooldown period, reclaim tokens n hours after unstaked
void patreostoken::unstake( name account, asset quantity )
{
    require_auth( account );
    auto sym = quantity.symbol;
    stats statstable( _self, sym.code().raw() );
    const auto& st = statstable.get( sym.code().raw(), Messages::TOKEN_DNE_YET );

    require_recipient( account );

    eosio_assert( quantity.is_valid(), "invalid quantity" );
    eosio_assert( quantity.amount > 0, "must transfer positive quantity" );
    eosio_assert( quantity.symbol == st.supply.symbol, "symbol precision mismatch" );

    sub_stake( account, quantity );
    add_balance( account, quantity, account );
}

void patreostoken::open( name owner, const symbol& symbol, name ram_payer )
{
   require_auth( ram_payer );

   auto sym_code_raw = symbol.code().raw();

   stats statstable( _self, sym_code_raw );
   const auto& st = statstable.get( sym_code_raw, Messages::SYMBOL_DNE );
   eosio_assert( st.supply.symbol == symbol, Messages::INVALID_SYMBOL );

   accounts acnts( _self, owner.value );
   auto it = acnts.find( sym_code_raw );
   if( it == acnts.end() ) {
      acnts.emplace( ram_payer, [&]( auto& a ){
        a.balance = asset{0, symbol};
      });
   }
}

void patreostoken::close( name owner, const symbol& symbol )
{
   require_auth( owner );
   accounts acnts( _self, owner.value );
   auto it = acnts.find( symbol.code().raw() );
   eosio_assert( it != acnts.end(), Messages::CLOSE_BALANCE_NO_EFFECT );
   eosio_assert( it->balance.amount == 0, Messages::CLOSE_BALANCE_NONZERO );
   acnts.erase( it );
}

void patreostoken::closestake( name owner, const symbol& symbol )
{
   require_auth( owner );
   stakes stakes_table( _self, owner.value );
   auto it = stakes_table.find( symbol.code().raw() );
   eosio_assert( it != stakes_table.end(), Messages::CLOSE_BALANCE_NO_EFFECT );
   eosio_assert( it->balance.amount == 0, Messages::CLOSE_BALANCE_NONZERO );
   stakes_table.erase( it );
}

EOSIO_DISPATCH( patreostoken, (create)(issue)(transfer)(open)(close)(closestake)(retire)(stake)(unstake) )

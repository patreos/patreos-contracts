#include <eosiolib/eosio.hpp>
#include <eosiolib/print.hpp>
#include <eosiolib/asset.hpp>
#include <eosiolib/transaction.hpp>
using namespace eosio;
using std::string;

// Needs permission set to eosio.code
class patreospayer : public eosio::contract {
private:
    /// @abi table pledge i64
    struct pledge {
        uint64_t key; // available_primary_key();
        //account_name from;
        account_name to;
        asset value;
        uint16_t days;
        time last_pledge; // now()

        uint64_t primary_key() const { return to; }

        EOSLIB_SERIALIZE(pledge, (to)(value)(days)(last_pledge))
    };

    typedef multi_index<N(pledges), pledge> pledges;

public:
    using contract::contract;

    /// @abi action
    void process(account_name from, account_name to, asset quantity, string memo)
    {
        //patreosnexus
        auto plegestable = pledges(N(patreosnexus), from);
        auto existing = plegestable.find(to);
        eosio_assert( existing != plegestable.end(), "pledge does not exist." );

        // TODO: Check that pledge quantity valid and set var if removed pledge

        //patreostoken
        double milliseconds = double(now() - existing->last_pledge);
        int days = (int) ( milliseconds / (1000 * 60 * 60 * 24) );
        if(days >= existing->days) {

          // Pay pledge amount
          action(permission_level{ _self, N(eosio.code) },
              N(patreostoken), N(pledge),
              std::make_tuple(from, to, quantity, memo)).send();

          // Increment execution_count and update last_pledge
          action(permission_level{ _self, N(eosio.code) },
              N(patreosnexus), N(pledge_paid),
              std::make_tuple(from, to)).send();
        }
    }
};

EOSIO_ABI(patreospayer, (process))

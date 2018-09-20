#include <eosiolib/asset.hpp>
#include <eosiolib/eosio.hpp>
#include <string>

using namespace eosio;
using std::string;

class patreosnexus : public eosio::contract {
private:

    struct publication {
        uint64_t item; // unique
        string title;
        string description;
        string url;

        uint64_t primary_key() const { return item; }
    };

    struct pledge_info {
        account_name to;
        asset quantity;
        uint16_t days;
        time last_pledge; // now()
        uint16_t execution_count;

        account_name primary_key() const { return to; }
    };

    struct profile {
        account_name owner;
        string name;
        string description;

        account_name primary_key() const { return owner; }
    };

    typedef eosio::multi_index<N(profiles), profile> profiles; // creator pays ram
    typedef eosio::multi_index<N(publications), publication> publications; // creator pays ram (optional)

    // Temp until offchain storage
    typedef eosio::multi_index<N(pledges), pledge_info> pledges; // we pay ram above certain PTR


    // patreostoken definitions
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

    typedef eosio::multi_index<N(liquidstake), account> liquidstake;
    typedef eosio::multi_index<N(stat), currency_stats> stats;

public:
    using contract::contract;

    patreosnexus(account_name self) : contract(self) {}

    void subscribe(account_name from, account_name to);

    void unsubscribe(account_name from, account_name to);

    void pledge(account_name from, account_name to, uint16_t days, asset quantity);

    void pledge_paid(account_name from, account_name to);

    void unpledge(account_name from, account_name to);

    void setprofile(account_name owner, profile _profile);

    void unsetprofile(account_name owner);

    void publish(account_name owner, publication _publication);

    void process(account_name from, account_name to, asset quantity);
};

#include <eosiolib/asset.hpp>
#include <eosiolib/eosio.hpp>
#include <string>

using namespace eosio;
using std::string;

class patreosnexus : public eosio::contract {
private:

    struct publication {
        account_name author;
        string title;
        string description;
        string url;

        account_name primary_key() const { return author; }
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

    // Temp tables until offchain storage

    struct user_sub {
        account_name owner;
        std::vector<account_name> subscribers;
        std::vector<account_name> subscribing;

        account_name primary_key() const { return owner; }
    };

    struct user_pledge {
        account_name owner;
        std::vector<account_name> pledgers;
        std::vector<account_name> pledging;

        account_name primary_key() const { return owner; }
    };

    typedef eosio::multi_index<N(profiles), profile> profiles; // creator pays ram
    typedef eosio::multi_index<N(publications), publication> publications; // we pay ram

    // Temp until offchain storage
    typedef eosio::multi_index<N(pledges), pledge_info> pledges;

    // Temp until offchain storage
    typedef eosio::multi_index<N(user_subs), user_sub> user_subs;
    typedef eosio::multi_index<N(user_pledges), user_pledge> user_pledges;

public:
    using contract::contract;

    patreosnexus(account_name self) : contract(self) { }

    void subscribe(account_name from, account_name to);

    void unsubscribe(account_name from, account_name to);

    void pledge(account_name from, account_name to, uint16_t days, asset quantity);

    void pledge_paid(account_name from, account_name to);

    void unpledge(account_name from, account_name to);

    void setprofile(account_name owner, profile _profile);

    void unsetprofile(account_name owner);

    void publish(account_name owner, publication _publication);

};

#include <eosio/eosio.hpp>

using namespace eosio;

// class [[eosio::contract]]
CONTRACT helloworld : public contract {
    public:
        using contract::contract;

        // [[eosio::action]] void
        ACTION welcome( name arrival ){
            require_auth( get_self() ); // provera korisnika preko user, get_self() za vlasnika contracta
            print ( "Welcome, ", arrival);
        }
};
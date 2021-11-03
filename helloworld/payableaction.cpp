#include <eosio/eosio.hpp>

CONTRACT payableaction : public contract {
    public:
        using contract::contract;


        [[eosio::on_notify("eosio.token::transfer")]]
        void depositnlock(name from, name to, eosio::asset quantity, std::string memo)
        {
            check ((to == get_self() && from != get_self()), "Ne placaj sebi!" );
            check (quantity.amount > 0, "Ne moze negativno");
            check (quantity.symbol == hodl_symbol, "Pogresan token!");
        }
        // KREIRATI akciju sutra deposit
}
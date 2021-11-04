#include <eosio/eosio.hpp>
#include <eosio/print.hpp>
#include <eosio/asset.hpp>
#include <eosio/system.hpp>
#include <eosio/singleton.hpp>
#include <eosio/binary_extension.hpp>

using namespace eosio;

CONTRACT hodl : public eosio::contract
{
    private:
        static const uint32_t the_party = 1528549200; // ovo je nekako datum tuesday, february 22, 2022 10:22:22 PM - promenjeno na 9 jun 2018 radi testa
        const symbol hodl_symbol; // symbol koji prima ovaj contract, u ovom slucaju "SYS"

        // tabela koja prati broj tokena koje je hodl contract primio
        TABLE balance
        {
            eosio::asset funds; // asset biblioteka iz eosio, tj. u pitanju je tip koji reprezentuje digitalni token
            uint64_t primary_key() const { return funds.symbol.raw(); } // symbol clan asset instance ce biti primarni kljuc. raw funkcija konvertuje symbol varijablu u unsigned integer kako bi mogla da se koristi kao prim kljuc
        };

        // singleton settings tabela, ovde ne treba primary key jer je signleton
        TABLE mysettings{
            double stkpercntage; // stake percentage
            uint32_t minunstkdays; // minimum unstake days
        } default_setings; // radi korisnosti

        // updateovanje tabele
        TABLE needsupdate {
            eosio::name olditem; // vec postojeci item u strukturi
            eosio::name newitem; // novi item u strukturi koji ocemo da dodamo u strukturu - kada bi bilo samo ovo, ne bi radilo
            eosio::binary_extension<eosio::name> binarykey; // neophodno za dodavanje newitema, u <> mora biti tip kojeg je newitem
            uint64_t primary_key() const { return olditem.value;  }
            uint64_t secondary_key() const { return newitem.value; } // ovo ce da radi
        };

        using singleton_type = eosio::singleton<"mysettings"_n, mysettings>;
        using balance_table = eosio::multi_index<"balance"_n, balance>;
        using needs_update = eosio::multi_index<"needsupdate"_n, needsupdate>;

        singleton_type settings_instance;

         // dohvatanje UTC vremena trenutne vremenske zone
        uint32_t now(){
            return current_time_point().sec_since_epoch();
        }

    public:
        using contract::contract;
        
        // konstruktor inicijalizuje hodl_symbol kao "SYS" token, a tu je i settings_instance koji se ugl moze def u konstruktoru
        hodl(name reciever, name code, datastream<const char *> ds):contract(reciever, code, ds), hodl_symbol("SYS", 4), settings_instance(reciever, reciever.value){}

        // ovaj atribut osigurava da bilo koja dolazna notifikacija biva prosledjena annotiranoj akciji, akko je notifikacija dosla od navedenog ugora i navedene akcije, u ovom slucaju eosio.token je ugovor, a akcija je transfer
        [[eosio::on_notify("eosio.token::transfer")]]
        ACTION deposit(name hodler, name to, eosio::asset quantity, std::string memo)
        {
            // provera da account ne transferuje samom sebi
            if (to != get_self() || hodler == get_self())
            {
                print("Ne greeduj i ne peri pare");
                return;
            }

            // provera da li je vreme za preuzimanje tokena proslo
            check(now() < the_party, "Kasnis budalo");
            // provera da li dolazni transfer ima validnu kolicinu tokena
            check(quantity.amount > 0, "Oces kurac");
            // provera da li transfer koristi token koji smo odredili u konstruktoru
            check(quantity.symbol == hodl_symbol, "Malo morgen moze taj token");

            // ako su provere prosle, apdejtuje se balans
            balance_table balance(get_self(), hodler.value);
            auto hodl_it = balance.find(hodl_symbol.raw());

            // ako ima, apdejtuj
            if (hodl_it != balance.end())
                balance.modify(hodl_it, get_self(), [&](auto &row){
                    row.funds += quantity;
                });
            else
                // else, ubaci podatak o balansu
                balance.emplace(get_self(), [&](auto &row) {
                    row.funds = quantity;
                });
        }

        // party akcija dopusta podizanje tokena nakon sto je konfigurisano the_party vreme proslo
        // party ima slicnu konstrukciju kao deposit akcija sa uslovima da account koji je depositovao bude i onaj koji podize, da pronadje zakljucani balans i da transferuje token sa hodl contracta na sam account
        ACTION party(name hodler)
        {
            // provera autoriteta
            require_auth(hodler);

            // provera da li je trenutno vreme preslo the_party vreme
            check (now() > the_party, "De si poso bre");

            balance_table balance(get_self(), hodler.value);
            auto hodl_it = balance.find(hodl_symbol.raw());

            // provera da je hodler u tabeli
            check(hodl_it != balance.end(), "Ne mozes da sedis sa nama");

            action(
                permission_level{get_self(), "active"_n},
                "eosio.token"_n,
                "transfer"_n,
                std::make_tuple(get_self(), hodler, hodl_it->funds, std::string("Zurkica ludilo!"))
            ).send();

            balance.erase(hodl_it);
        }

        // menjanje settingsa
        ACTION modsettings (
            double stkpercntage,
            uint32_t minunstkdays
        ) {
            auto settings_stored = settings_instance.get_or_create(get_self(), default_setings); // ili gettuje i modifikuje ili kreira red singleton klase ako ne postoji
            settings_stored.stkpercntage = stkpercntage;
            settings_stored.minunstkdays = minunstkdays;
            settings_instance.set(settings_stored, get_self()); // settuje ucitano u default settings
        }

        // dohvatanje settingsa
        [[eosio::action]] uint32_t getsettings ()
        {
            if (settings_instance.exists())
            {
                print(settings_instance.get().minunstkdays);
                return settings_instance.get().minunstkdays;
            } 
            else 
                return -1;
        }

        ACTION addnewrow (eosio::name olditem, eosio::binary_extension<eosio::name> newitem) // da bi radilo, newitem se mora wrappovati u eosio::binary_extension
        {
            needs_update _needsupdate(get_self(), olditem.value);
            _needsupdate.emplace(get_self(), [&](auto& row){
                row.olditem = olditem;
                if (newitem){
                    row.newitem = newitem;
                }
                else {
                    row.newitem = name("nullfill");
                }
            });
        }
};
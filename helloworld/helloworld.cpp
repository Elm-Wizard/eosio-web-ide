#include <eosio/eosio.hpp>
#include <eosio/system.hpp>

using namespace eosio;
using namespace std;

// class [[eosio::contract]]
CONTRACT helloworld : public contract {
    public:
        using contract::contract;

        // [[eosio::action]] void
        ACTION addrecord( int128_t id, string username, string userdescrip, uint64_t quantity ){
            check (has_auth(name( "aksa")), "Nisi  nas vlasnik"); //check je kao assert 
            //require_auth( name ( "aksa" )); // provera korisnika preko user, get_self() za vlasnika contracta
            //check( user == name( "aksa" ), "Ne lazi budalo");

            record_table _recordtable(get_self(), get_self().value); //get_self() je code, a get_self().value je scope, tablename i key su definisani u typdef
             
            
            // ako je id -1, dodajemo novog usera
            if (id == -1){
                _recordtable.emplace(get_self(), [&](auto& new_row){//prvi argument je payer, auto& new_row je callback, tj. lambda funkcija koja modifikuje red koji passujemo po referenci
                    new_row.id = _recordtable.available_primary_key();
                    new_row.username = username;
                    new_row.userdescrip = userdescrip;
                    new_row.quantity = quantity;
                });
            } else {
                //auto record = recortable.get(id.value, "Nema recorda!"); // get metoda vraca referencu na data record ako ga nadje, ako ne nadje, onda prekida izvrsavanje koda...drugi arg je error string 
                auto itr = _recordtable.find (id); // vraca iterator koji pokazuje na red koji sadrzi index id tj. key, onda treba proveriti da li je end state ili korisnik postoji
                check (itr != _recordtable.end(), "Nema usera sa datim id-om.");

                _recordtable.modify(itr, get_self(), [&](auto& row_to_modify){ // ako ima user, menjamo usera preko funkcije modify, ova funkcija ima dodatan arg u vidu itr, koji pokazuje mesto recorda koji menjamo, payer moze biti i prazan ako se zadrzava isti payer
                    row_to_modify.quantity += quantity; 
                });
                
            }
        }

        ACTION logtime(){
            require_auth ( name("aksa"));
            record_table _recordtable(get_self(), get_self().value);
            auto sec_idx = _recordtable.get_index<name("timestamps")>();
            for (auto itr = sec_idx.begin(); itr != sec_idx.end(); itr++)
                {
                    eosio::print("Record pristupa korisnika: ", itr->username, ", timestamp:", itr->timestamp, "\n");
                }
        }

        ACTION removeuser( uint128_t id, uint64_t quantity) {
            require_auth( name( "aksa" ));
            record_table _recordtable(get_self(), get_self().value);

            //pronadje usera
            //ako nema - error
            auto itr = _recordtable.find (id);
            check (itr != _recordtable.end(), "Nema usera sa datim id-om.");
            //ako je quantity 0, obrisi row
            //ako je manje od 1, error
            if ((itr->quantity - quantity) == 0){
                _recordtable.erase(itr);
                print ("Uspesno obrisan user ", id, " x", quantity);
            } else {
                check((quantity >= itr->quantity) > 0, "Pokusavaj negativnog kvantiteta");
                _recordtable.modify(itr, get_self(), [&](auto& row_to_modify){
                    row_to_modify.quantity -= quantity;
                });
            }
            //ako postoji, smanji quantity
        }
        // kompletni CRUD se sastoji iz emplace(), find() / get(), modify() i erase()

        /*TABLE surveillance_record {
            name subject;
            eosio::time_point timestamp;

            uint64_t primary_key() const { return subject.value; }
            EOSLIB_SERIALIZE(surveillance_record, (subject)(timestamp)) // serijalizacija podataka jer treba u binary formi da se storuje
        }; */

        TABLE record {
            uint64_t id = 0;
            uint64_t timestamp;
            string username;
            string userdescrip;
            uint64_t quantity;
            name owner = name( "aksa" );

            auto primary_key() const {return id;}
            uint64_t by_secondary() const {return timestamp;}
        };



        typedef eosio::multi_index<name("recordtable"), record, eosio::indexed_by<
            name("timestamps"),
            eosio::const_mem_fun<
                record,
                uint64_t,
                &record::by_secondary
                >
            >
        > record_table; // definisanje multi_indexa, prvo ime tabele, pa struktura tabele, ako imamo jos jedan index, onda se dodaje treci arg indexed_by
        // prvi arg za indexed_by je ime za secondary index, drugi je const_mem_fun kojem prosledjujemo strukturu tabele (record), tip indeksa i konstantnu fkciju za drugi indeks


};
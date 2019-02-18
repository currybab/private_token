#include <eosiolib/eosio.hpp>
#include <eosiolib/asset.hpp>
#include <string>

using namespace eosio;

using std::string;

class [[eosio::contract]] fluid_token : public contract {
  public:
    
    fluid_token(name receiver, name code, datastream<const char*> ds): contract(receiver, code, ds) {}

    [[eosio::action]]
    void create(name issuer, asset maximum_supply);

    [[eosio::action]]
    void issue(name to, asset quantity, string memo);

    [[eosio::action]]
    void transfer(name from, name to, asset quantity, string memo);

    [[eosio::action]]
    void withdraw(name from, name to, asset quantity, string memo);

    void eostransfer(uint64_t sender, uint64_t receiver);

    inline asset get_supply(symbol sym) const;

    inline asset get_balance(name owner, symbol sym) const;

  private:
    struct [[eosio::table]] account {
        asset balance;

        uint64_t primary_key() const { return balance.symbol.raw(); }
    };

    struct [[eosio::table]] currency_stats {
        asset supply;
        asset max_supply;
        name issuer;

        uint64_t primary_key() const { return supply.symbol.raw(); }
    };

    typedef eosio::multi_index<name("accounts"), account> accounts;
    typedef eosio::multi_index<name("stat"), currency_stats> stats;

    void sub_balance(name owner, asset value);

    void add_balance(name owner, asset value, name ram_payer);

  public:
    struct transfer_args {
        name from;
        name to;
        asset quantity;
        string memo;
    };
};

asset fluid_token::get_supply(symbol sym) const {
    stats statstable(_self, sym.raw());
    const auto &st = statstable.get(sym.raw());
    return st.supply;
};

asset fluid_token::get_balance(name owner, symbol sym) const {
    accounts accountstable(_self, owner.value);
    const auto &ac = accountstable.get(sym.raw());
    return ac.balance;
};

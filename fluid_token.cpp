#include <eosiolib/dispatcher.hpp>
#include "fluid_token.hpp"

using namespace eosio;

void fluid_token::create(name issuer, asset maximum_supply) {
    require_auth(_self);

    auto sym = maximum_supply.symbol;
    eosio_assert(sym.is_valid(), "invalid symbol name");
    eosio_assert(maximum_supply.is_valid(), "invalid supply");
    eosio_assert(maximum_supply.amount > 0, "max-supply must be positive");

    stats statstable(_self, sym.raw());
    auto existing = statstable.find(sym.raw());
    eosio_assert(existing == statstable.end(), "token with symbol already exists");

    statstable.emplace(_self, [&](auto &s) {
        s.supply.symbol = maximum_supply.symbol;
        s.max_supply = maximum_supply;
        s.issuer = issuer;
    });
};

void fluid_token::issue(name to, asset quantity, string memo) {
    auto sym = quantity.symbol;
    eosio_assert(sym.is_valid(), "invalid symbol name");
    eosio_assert(memo.size() <= 256, "memo has more than 256 bytes");

    auto sym_name = sym.raw();
    stats statstable(_self, sym_name);
    auto existing = statstable.find(sym_name);
    eosio_assert(existing != statstable.end(), "token with symbol does not exist, create token before issue");
    const auto &st = *existing;

    require_auth(st.issuer);
    eosio_assert(quantity.is_valid(), "invalid quantity");
    eosio_assert(quantity.amount > 0, "must issue positive quantity");

    eosio_assert(quantity.symbol == st.supply.symbol, "symbol precision mismatch");
    eosio_assert(quantity.amount <= st.max_supply.amount - st.supply.amount, "quantity exceeds available supply");

    statstable.modify(st, _self, [&](auto &s) {
        s.supply += quantity;
    });

    add_balance(st.issuer, quantity, st.issuer);

    if (to != st.issuer) {
        SEND_INLINE_ACTION(*this, transfer, {st.issuer, name("active")}, {st.issuer, to, quantity, memo});
    }
};

void fluid_token::transfer(name from, name to, asset quantity, string memo) {
    eosio_assert(from != to, "cannot transfer to self");
    require_auth(from);
    eosio_assert(is_account(to), "to account does not exist");
    auto sym = quantity.symbol.raw();
    stats statstable(_self, sym);
    const auto &st = statstable.get(sym);

    require_recipient(from);
    require_recipient(to);

    eosio_assert(quantity.is_valid(), "invalid quantity");
    eosio_assert(quantity.amount > 0, "must transfer positive quantity");
    eosio_assert(quantity.symbol == st.supply.symbol, "symbol precision mismatch");
    eosio_assert(memo.size() <= 256, "memo has more than 256 bytes");

    sub_balance(from, quantity);
    add_balance(to, quantity, from);
};

void fluid_token::withdraw(name from, name to, asset quantity, string memo) {
    
};

void fluid_token::eostransfer(uint64_t sender, uint64_t receiver) {
    auto transfer_data = unpack_action_data<transfer_args>();

    if (transfer_data.from == _self) {
        return;
    }
};

void fluid_token::sub_balance(name owner, asset value) {
    accounts from_acnts(_self, owner.value);

    const auto &from = from_acnts.get(value.symbol.raw(), "no balance object found");
    eosio_assert(from.balance.amount >= value.amount, "overdrawn balance");

    if (from.balance.amount == value.amount) {
        from_acnts.erase(from);
    }
    else {
        from_acnts.modify(from, owner, [&](auto &a) {
            a.balance -= value;
        });
    }
};

void fluid_token::add_balance(name owner, asset value, name ram_payer) {
    accounts to_acnts(_self, owner.value);
    auto to = to_acnts.find(value.symbol.raw());
    if (to == to_acnts.end()) {
        to_acnts.emplace(ram_payer, [&](auto &a) {
            a.balance = value;
        });
    }
    else {
        to_acnts.modify(to, _self, [&](auto &a) {
            a.balance += value;
        });
    }
};


#define EOSIO_DISPATCH_CUSTOM( TYPE, MEMBERS ) \
extern "C" { \
    void apply( uint64_t receiver, uint64_t code, uint64_t action ) { \
        auto self = receiver; \
        if(code == name("eosio.token").value && action == name("transfer").value) { \
            execute_action(name(self), name(code), &fluid_token::eostransfer); \
            return; \
        } \
        eosio_assert(action != name("eostransfer").value, "eostransfer cannot be execute by itself"); \
        switch( action ) { \
            EOSIO_DISPATCH_HELPER( TYPE, MEMBERS ) \
        } \
    } \
}

EOSIO_DISPATCH_CUSTOM(fluid_token, (create)(issue)(eostransfer)(transfer)(withdraw))

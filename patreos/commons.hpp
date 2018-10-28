#pragma once

#include <eosiolib/eosio.hpp>
#include <eosiolib/asset.hpp>
#include <cmath>

// delay in seconds
const uint64_t SECONDS_IN_DAY = 86400;
//const uint64_t PATREOS_UNSTAKE_DELAY = SECONDS_IN_DAY * 3;
const uint64_t PATREOS_UNSTAKE_DELAY = 15; // for testing

const double eos_fee_double = 0.1;
const double patreos_fee_double = 0; // always no fees for PTR

const uint64_t EOS_PRECISION = 4;
const eosio::symbol_type EOS_SYMBOL = S(EOS_PRECISION, EOS);
const eosio::symbol_type PTR_SYMBOL = S(EOS_PRECISION, PTR);

const eosio::asset patreos_fee = eosio::asset((uint64_t)(std::pow(10, EOS_PRECISION) * patreos_fee_double), PTR_SYMBOL);
const eosio::asset eos_fee = eosio::asset((uint64_t)(std::pow(10, EOS_PRECISION) * eos_fee_double), EOS_SYMBOL);

const uint64_t EOS_TOKEN_CODE = N(eosio.token);
const uint64_t PATREOS_TOKEN_CODE = N(patreostoken);
const uint64_t PATREOS_NEXUS_CODE = N(patreosnexus);
const uint64_t PATREOS_VAULT_CODE = N(patreosvault);
const uint64_t PATREOS_BLURB_CODE = N(patreosblurb);


inline static bool is_supported_asset(const eosio::asset& asset) {
  if(asset.symbol == EOS_SYMBOL) {
    return true;
  } else if (asset.symbol == PTR_SYMBOL) {
    return true;
  } else {
    return false;
  }
}

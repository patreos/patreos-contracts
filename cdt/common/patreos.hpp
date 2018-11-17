#pragma once

#include <eosiolib/eosio.hpp>
#include <eosiolib/asset.hpp>
#include <cmath>

// delay in seconds
const uint64_t SECONDS_IN_DAY = 86400;

//const uint64_t PATREOS_UNSTAKE_DELAY = SECONDS_IN_DAY * 3;
const uint64_t PATREOS_UNSTAKE_DELAY = 15; // for testing

const uint64_t EOS_PRECISION = 4;

const eosio::symbol EOS_SYMBOL = eosio::symbol("EOS", EOS_PRECISION);
const eosio::symbol PTR_SYMBOL = eosio::symbol("PATR", EOS_PRECISION);

// Contracts
const eosio::name EOS_TOKEN_CODE = "eosio.token"_n;
const eosio::name PATREOS_TOKEN_CODE = "patreostoken"_n;
const eosio::name PATREOS_NEXUS_CODE = "patreosnexus"_n;
const eosio::name PATREOS_VAULT_CODE = "patreosvault"_n;
const eosio::name PATREOS_BLURB_CODE = "patreosblurb"_n;

// Permissions
const eosio::name EOS_CODE_PERMISSION = "eosio.code"_n;

// Fees
const double eos_fee_double = 0.1;
const double patreos_fee_double = 0; // always no fees for PTR
const eosio::asset eos_fee = eosio::asset((uint64_t)(std::pow(10, EOS_PRECISION) * eos_fee_double), EOS_SYMBOL);
const eosio::asset patreos_fee = eosio::asset((uint64_t)(std::pow(10, EOS_PRECISION) * patreos_fee_double), PTR_SYMBOL);

// Min pledges
const double min_pledge_eos_double = 0.2;
const double min_pledge_ptr_double = 50;
const eosio::asset min_pledge_eos = eosio::asset((uint64_t)(std::pow(10, EOS_PRECISION) * min_pledge_eos_double), EOS_SYMBOL);
const eosio::asset min_pledge_ptr = eosio::asset((uint64_t)(std::pow(10, EOS_PRECISION) * min_pledge_ptr_double), PTR_SYMBOL);

const double SECONDS_IN_YEAR = 31557600;
const double BLOCKS_PER_SECOND = 2;

inline static bool is_supported_asset(const eosio::asset& asset) {
  if(asset.symbol == EOS_SYMBOL) {
    return true;
  } else if (asset.symbol == PTR_SYMBOL) {
    return true;
  } else {
    return false;
  }
}

inline static bool is_pledge_cycle_valid(uint32_t seconds) {
  uint32_t days = seconds / SECONDS_IN_DAY;
  //return days == 30 || days == 7;
  return true;
}

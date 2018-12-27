#pragma once

#include <eosiolib/eosio.hpp>
#include <eosiolib/asset.hpp>
#include <cmath>

// delay in seconds
const uint64_t SECONDS_IN_DAY = 86400;
const uint64_t SECONDS_IN_YEAR = 31557600;
const uint64_t BLOCKS_PER_SECOND = 2;

const uint64_t EOS_PRECISION = 4;
const eosio::symbol EOS_SYMBOL = eosio::symbol("EOS", EOS_PRECISION);

// Contracts
const eosio::name EOS_TOKEN_CODE = "eosio.token"_n;
const eosio::name RECURRING_PAY_CODE = "recurringpay"_n;
const eosio::name COLLECTION_ACCOUNT = "recurringpay"_n;

// Actions
const eosio::name EOS_TRANSFER_ACTION = "transfer"_n;

// Permissions
const eosio::name EOS_CODE_PERMISSION = "eosio.code"_n;
const eosio::name EOS_ACTIVE_PERMISSION = "active"_n;

// Costs
const eosio::asset reg_cost = eosio::asset(
  (uint64_t) ( std::pow(10, EOS_PRECISION) * 1 ),
  EOS_SYMBOL
);

inline static bool is_pledge_cycle_valid( uint32_t seconds ) {
  if(seconds <= 0) {
    return false;
  }
  uint32_t days = seconds / SECONDS_IN_DAY;
  //return days == 30 || days == 7;
  return true;
}

/*
 * This file is part of the TREZOR project.
 *
 * Copyright (C) 2014 Pavol Rusnak <stick@satoshilabs.com>
 *
 * This library is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this library.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "keepkey/firmware/coins.h"

#include "keepkey/board/util.h"

#include <inttypes.h>
#include <string.h>
#include <stdio.h>

#define SECP256K1_STRING "secp256k1"

const CoinType coins[COINS_COUNT] = {
#define X(\
HAS_COIN_NAME, COIN_NAME, \
HAS_COIN_SHORTCUT, COIN_SHORTCUT, \
HAS_ADDRESS_TYPE, ADDRESS_TYPE, \
HAS_MAXFEE_KB, MAXFEE_KB, \
HAS_ADDRESS_TYPE_P2SH, ADDRESS_TYPE_P2SH, \
HAS_SIGNED_MESSAGE_HEADER, SIGNED_MESSAGE_HEADER, \
HAS_BIP44_ACCOUNT_PATH, BIP44_ACCOUNT_PATH, \
HAS_FORKID, FORKID, \
HAS_DECIMALS, DECIMALS, \
HAS_CONTRACT_ADDRESS, CONTRACT_ADDRESS, \
HAS_XPUB_MAGIC, XPUB_MAGIC, \
HAS_SEGWIT, SEGWIT , \
HAS_FORCE_BIP143, FORCE_BIP143, \
HAS_CURVE_NAME, CURVE_NAME, \
HAS_CASHADDR_PREFIX, CASHADDR_PREFIX, \
HAS_BECH32_PREFIX, BECH32_PREFIX, \
HAS_DECRED, DECRED, \
HAS_XPUB_MAGIC_SEGWIT_P2SH, XPUB_MAGIC_SEGWIT_P2SH, \
HAS_XPUB_MAGIC_SEGWIT_NATIVE, XPUB_MAGIC_SEGWIT_NATIVE \
) \
    { HAS_COIN_NAME, COIN_NAME, \
      HAS_COIN_SHORTCUT, COIN_SHORTCUT, \
      HAS_ADDRESS_TYPE, ADDRESS_TYPE, \
      HAS_MAXFEE_KB, MAXFEE_KB, \
      HAS_ADDRESS_TYPE_P2SH, ADDRESS_TYPE_P2SH, \
      HAS_SIGNED_MESSAGE_HEADER, SIGNED_MESSAGE_HEADER, \
      HAS_BIP44_ACCOUNT_PATH, BIP44_ACCOUNT_PATH, \
      HAS_FORKID, FORKID, \
      HAS_DECIMALS, DECIMALS, \
      HAS_CONTRACT_ADDRESS, CONTRACT_ADDRESS, \
      HAS_XPUB_MAGIC, XPUB_MAGIC, \
      HAS_SEGWIT, SEGWIT , \
      HAS_FORCE_BIP143, FORCE_BIP143, \
      HAS_CURVE_NAME, CURVE_NAME, \
      HAS_CASHADDR_PREFIX, CASHADDR_PREFIX, \
      HAS_BECH32_PREFIX, BECH32_PREFIX, \
      HAS_DECRED, DECRED, \
      HAS_XPUB_MAGIC_SEGWIT_P2SH, XPUB_MAGIC_SEGWIT_P2SH, \
      HAS_XPUB_MAGIC_SEGWIT_NATIVE, XPUB_MAGIC_SEGWIT_NATIVE },
    #include "keepkey/firmware/coins.def"

#define X(INDEX, NAME, SYMBOL, DECIMALS, CONTRACT_ADDRESS) \
    {   \
      true, (#NAME),                     /* has_coin_name, coin_name*/ \
      true, (#SYMBOL),                   /* has_coin_shortcut, coin_shortcut*/ \
      false, NA,                         /* has_address_type, address_type*/ \
      true, 100000,                      /* has_maxfee_kb, maxfee_kb*/ \
      false, NA,                         /* has_address_type_p2sh, address_type_p2sh*/ \
      false, "",                         /* has_signed_message_header, signed_message_header*/ \
      true, 0x8000003C,                  /* has_bip44_account_path, bip44_account_path*/ \
      true, 1,                           /* has_forkid, forkid*/ \
      true, (DECIMALS),                  /* has_decimals, decimals*/ \
      true, {20, {(CONTRACT_ADDRESS)}},  /* has_contract_address, contract_address*/ \
      false, 0,                          /* has_xpub_magic, xpub_magic*/ \
      false, false,                      /* has_segwit, segwit */ \
      false, false,                      /* has_force_bip143, force_bip143*/ \
      true, SECP256K1_STRING,            /* has_curve_name, curve_name*/ \
      false, "",                         /* has_cashaddr_prefix, cashaddr_prefix*/ \
      false, "",                         /* has_bech32_prefix, bech32_prefix*/ \
      false, false,                      /* has_decred, decred */ \
      false, 0,                          /* has_xpub_magic_segwit_p2sh, xpub_magic_segwit_p2sh*/ \
      false, 0,                          /* has_xpub_magic_segwit_native, xpub_magic_segwit_native*/ \
    },
    #include "keepkey/firmware/tokens.def"
};

_Static_assert(sizeof(coins) / sizeof(coins[0]) == COINS_COUNT,
               "Update COINS_COUNT to match the size of the coin table");

// Borrowed from fsm_msg_coin.h
// PLEASE keep these in sync.
static bool path_mismatched(const CoinType *coin, const uint32_t *address_n,
                            uint32_t address_n_count, bool whole_account)
{
	bool mismatch = false;

	// m : no path
	if (address_n_count == 0) {
		return false;
	}

	// m/44' : BIP44 Legacy
	// m / purpose' / bip44_account_path' / account' / change / address_index
	if (address_n[0] == (0x80000000 + 44)) {
		mismatch |= (address_n_count != (whole_account ? 3 : 5));
		mismatch |= (address_n[1] != coin->bip44_account_path);
		mismatch |= (address_n[2] & 0x80000000) == 0;
		if (!whole_account) {
			mismatch |= (address_n[3] & 0x80000000) == 0x80000000;
			mismatch |= (address_n[4] & 0x80000000) == 0x80000000;
		}
		return mismatch;
	}

	// m/45' - BIP45 Copay Abandoned Multisig P2SH
	// m / purpose' / cosigner_index / change / address_index
	if (address_n[0] == (0x80000000 + 45)) {
		mismatch |= (address_n_count != 4);
		mismatch |= (address_n[1] & 0x80000000) == 0x80000000;
		mismatch |= (address_n[2] & 0x80000000) == 0x80000000;
		mismatch |= (address_n[3] & 0x80000000) == 0x80000000;
		return mismatch;
	}

	// m/48' - BIP48 Copay Multisig P2SH
	// m / purpose' / bip44_account_path' / account' / change / address_index
	if (address_n[0] == (0x80000000 + 48)) {
		mismatch |= (address_n_count != (whole_account ? 3 : 5));
		mismatch |= (address_n[1] != coin->bip44_account_path);
		mismatch |= (address_n[2] & 0x80000000) == 0;
		if (!whole_account) {
			mismatch |= (address_n[3] & 0x80000000) == 0x80000000;
			mismatch |= (address_n[4] & 0x80000000) == 0x80000000;
		}
		return mismatch;
	}

	// m/49' : BIP49 SegWit
	// m / purpose' / bip44_account_path' / account' / change / address_index
	if (address_n[0] == (0x80000000 + 49)) {
		mismatch |= !coin->has_segwit || !coin->segwit;
		mismatch |= !coin->has_address_type_p2sh;
		mismatch |= (address_n_count != (whole_account ? 3 : 5));
		mismatch |= (address_n[1] != coin->bip44_account_path);
		mismatch |= (address_n[2] & 0x80000000) == 0;
		if (!whole_account) {
			mismatch |= (address_n[3] & 0x80000000) == 0x80000000;
			mismatch |= (address_n[4] & 0x80000000) == 0x80000000;
		}
		return mismatch;
	}

	// m/84' : BIP84 Native SegWit
	// m / purpose' / bip44_account_path' / account' / change / address_index
	if (address_n[0] == (0x80000000 + 84)) {
		mismatch |= !coin->has_segwit || !coin->segwit;
		mismatch |= !coin->has_bech32_prefix;
		mismatch |= (address_n_count != (whole_account ? 3 : 5));
		mismatch |= (address_n[1] != coin->bip44_account_path);
		mismatch |= (address_n[2] & 0x80000000) == 0;
		if (!whole_account) {
			mismatch |= (address_n[3] & 0x80000000) == 0x80000000;
			mismatch |= (address_n[4] & 0x80000000) == 0x80000000;
		}
		return mismatch;
	}

	// Special case (not needed in the other copy of this function):
	if (address_n_count == 5 &&
		(strncmp(coin->coin_name, ETHEREUM, strlen(ETHEREUM)) == 0 ||
		 strncmp(coin->coin_name, ETHEREUM_CLS, sizeof(ETHEREUM_CLS)) == 0)) {
		// Check that the path is m/44'/bip44_account_path/y/0/0
		if (address_n[3] != 0)
			return true;
		if (address_n[4] != 0)
			return true;
	}

	return false;
}

bool bip32_path_to_string(char *str, size_t len, const uint32_t *address_n,
                          size_t address_n_count) {
    memset(str, 0, len);

    int cx = snprintf(str, len, "m");
    if (cx < 0 || len <= (size_t)cx)
        return false;
    str += cx;
    len -= cx;

    for (size_t i = 0; i < address_n_count; i++) {
        cx = snprintf(str, len, "/%" PRIu32, address_n[i] & 0x7fffffff);
        if (cx < 0 || len <= (size_t)cx)
            return false;
        str += cx;
        len -= cx;

        if ((address_n[i] & 0x80000000) == 0x80000000) {
            cx = snprintf(str, len, "'");
            if (cx < 0 || len <= (size_t)cx)
                return false;
            str += cx;
            len -= cx;
        }
    }

    return true;
}

const CoinType *coinByShortcut(const char *shortcut)
{
    if(!shortcut) { return 0; }

    int i;

    for(i = 0; i < COINS_COUNT; i++)
    {
        if(strncasecmp(shortcut, coins[i].coin_shortcut,
                       sizeof(coins[i].coin_shortcut)) == 0)
        {
            return &coins[i];
        }
    }

    return 0;
}

const CoinType *coinByName(const char *name)
{
    if(!name) { return 0; }

    int i;

    for(i = 0; i < COINS_COUNT; i++)
    {
        if(strncasecmp(name, coins[i].coin_name,
                       sizeof(coins[i].coin_name)) == 0)
        {
            return &coins[i];
        }
    }

    return 0;
}

const CoinType *coinByAddressType(uint32_t address_type)
{
    int i;

    for(i = 0; i < COINS_COUNT; i++)
    {
        if(address_type == coins[i].address_type)
        {
            return &coins[i];
        }
    }

    return 0;
}

const CoinType *coinBySlip44(uint32_t bip44_account_path)
{
    for (int i = 0; i < COINS_COUNT; i++) {
        if (bip44_account_path == coins[i].bip44_account_path) {
            return &coins[i];
        }
    }
    return 0;
}

/*
 * coin_amnt_to_str() - convert decimal coin amount to string for display
 *
 * INPUT -
 *      - coin: coin to use to determine bip44 path
 *      - amnt - coing amount in decimal
 *      - *buf - output buffer for coin amount in string
 *      - len - length of buffer
 * OUTPUT -
 *     none
 *
 */
void coin_amnt_to_str(const CoinType *coin, uint64_t amnt, char *buf, int len)
{
    uint64_t coin_fraction_part, coin_whole_part;
    int i;
    char buf_fract[10];

    memset(buf, 0, len);
    memset(buf_fract, 0, 10);

    /*Seperate amount to whole and fraction (amount = whole.fraction)*/
    coin_whole_part = amnt / COIN_FRACTION ;
    coin_fraction_part = amnt % COIN_FRACTION;

    /* Convert whole value to string */
    if(coin_whole_part > 0)
    {
        dec64_to_str(coin_whole_part, buf);
        buf[strlen(buf)] = '.';
    }
    else
    {
        strncpy(buf, "0.", 2);
    }

    /* Convert Fraction value to string */
    if(coin_fraction_part > 0)
    {
        dec64_to_str(coin_fraction_part, buf_fract);

        /* Add zeros after decimal */
        i = 8 - strlen(buf_fract);
        while(i)
        {
            buf[strlen(buf)+i-1] = '0';
            i--;
        }
        /*concantenate whole and fraction part of string */
        strncpy(buf+strlen(buf), buf_fract, strlen(buf_fract));

        /* Drop least significant zeros in fraction part to shorten display*/
        i = strlen(buf);
        while(buf[i-1] == '0')
        {
            buf[i-1] = 0;
            i--;
        }
    }
    else
    {
        buf[strlen(buf)] = '0';
    }
    /* Added coin type to amount */
    if(coin->has_coin_shortcut)
    {
        buf[strlen(buf)] = ' ';
        strncpy(buf + strlen(buf), coin->coin_shortcut, strlen(coin->coin_shortcut));
    }
}

static const char *account_prefix(const CoinType *coin,
                                  const uint32_t *address_n,
                                  size_t address_n_count,
                                  bool whole_account) {
    if (!coin->has_segwit || !coin->segwit)
        return "";

    if (address_n_count < (whole_account ? 3 : 5))
        return NULL;

    uint32_t purpose = address_n[address_n_count - (whole_account ? 3 : 5)];

    if (purpose == (0x80000000 | 44))
        return "Legacy ";

    if (purpose == (0x80000000 | 49))
        return "SegWit ";

    if (purpose == (0x80000000 | 84))
        return "";

    return NULL;
}

bool bip32_node_to_string(char *node_str, size_t len, const CoinType *coin,
                          const uint32_t *address_n, size_t address_n_count,
                          bool whole_account)
{
    if (address_n_count != 3 && address_n_count != 5)
        return false;

    // Don't display this way for change addresses, discouraging their use in GetAddress.
    if (!whole_account) {
        if (address_n_count != 5)
            return false;

        if (address_n[3] != 0)
            return false;
    }

    if (path_mismatched(coin, address_n, address_n_count, whole_account))
        return false;

    const char *prefix = account_prefix(coin, address_n, address_n_count, whole_account);
    if (!prefix)
        return false;

    // If it is a token, we still refer to the destination as an Ethereum account.
    bool is_token = coin->has_contract_address;
    const char *coin_name = is_token ? "Ethereum" : coin->coin_name;

    if (whole_account || isEthereumLike(coin_name)) {
        snprintf(node_str, len, "%s%s Account #%" PRIu32, prefix, coin_name,
                 address_n[2] & 0x7ffffff);
    } else {
        snprintf(node_str, len, "%s%s Account #%" PRIu32 "\nAddress #%" PRIu32, prefix, coin_name,
                 address_n[2] & 0x7ffffff, address_n[4]);
    }

    return true;
}

bool isEthereumLike(const char *coin_name)
{
    if (strcmp(coin_name, ETHEREUM) == 0)
        return true;

    if (strcmp(coin_name, ETHEREUM_CLS) == 0)
        return true;

    return false;
}

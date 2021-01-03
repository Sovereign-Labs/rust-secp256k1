/***********************************************************************
 * Copyright (c) 2020 Pieter Wuille                                    *
 * Distributed under the MIT software license, see the accompanying    *
 * file COPYING or https://www.opensource.org/licenses/mit-license.php.*
 ***********************************************************************/

#ifndef _SECP256K1_MODULE_SCHNORRSIG_TESTS_EXHAUSTIVE_
#define _SECP256K1_MODULE_SCHNORRSIG_TESTS_EXHAUSTIVE_

#include "include/secp256k1_schnorrsig.h"
#include "src/modules/schnorrsig/main_impl.h"

static const unsigned char invalid_pubkey_bytes[][32] = {
    /* 0 */
    {
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
    },
    /* 2 */
    {
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 2
    },
    /* order */
    {
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
        ((EXHAUSTIVE_TEST_ORDER + 0UL) >> 24) & 0xFF,
        ((EXHAUSTIVE_TEST_ORDER + 0UL) >> 16) & 0xFF,
        ((EXHAUSTIVE_TEST_ORDER + 0UL) >> 8) & 0xFF,
        (EXHAUSTIVE_TEST_ORDER + 0UL) & 0xFF
    },
    /* order + 1 */
    {
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
        ((EXHAUSTIVE_TEST_ORDER + 1UL) >> 24) & 0xFF,
        ((EXHAUSTIVE_TEST_ORDER + 1UL) >> 16) & 0xFF,
        ((EXHAUSTIVE_TEST_ORDER + 1UL) >> 8) & 0xFF,
        (EXHAUSTIVE_TEST_ORDER + 1UL) & 0xFF
    },
    /* field size */
    {
        0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
        0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFE, 0xFF, 0xFF, 0xFC, 0x2F
    },
    /* field size + 1 (note that 1 is legal) */
    {
        0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
        0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFE, 0xFF, 0xFF, 0xFC, 0x30
    },
    /* 2^256 - 1 */
    {
        0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
        0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF
    }
};

#define NUM_INVALID_KEYS (sizeof(invalid_pubkey_bytes) / sizeof(invalid_pubkey_bytes[0]))

static int rustsecp256k1_v0_4_0_hardened_nonce_function_smallint(unsigned char *nonce32, const unsigned char *msg32,
                                                      const unsigned char *key32, const unsigned char *xonly_pk32,
                                                      const unsigned char *algo16, void* data) {
    rustsecp256k1_v0_4_0_scalar s;
    int *idata = data;
    (void)msg32;
    (void)key32;
    (void)xonly_pk32;
    (void)algo16;
    rustsecp256k1_v0_4_0_scalar_set_int(&s, *idata);
    rustsecp256k1_v0_4_0_scalar_get_b32(nonce32, &s);
    return 1;
}

static void test_exhaustive_schnorrsig_verify(const rustsecp256k1_v0_4_0_context *ctx, const rustsecp256k1_v0_4_0_xonly_pubkey* pubkeys, unsigned char (*xonly_pubkey_bytes)[32], const int* parities) {
    int d;
    uint64_t iter = 0;
    /* Iterate over the possible public keys to verify against (through their corresponding DL d). */
    for (d = 1; d <= EXHAUSTIVE_TEST_ORDER / 2; ++d) {
        int actual_d;
        unsigned k;
        unsigned char pk32[32];
        memcpy(pk32, xonly_pubkey_bytes[d - 1], 32);
        actual_d = parities[d - 1] ? EXHAUSTIVE_TEST_ORDER - d : d;
        /* Iterate over the possible valid first 32 bytes in the signature, through their corresponding DL k.
           Values above EXHAUSTIVE_TEST_ORDER/2 refer to the entries in invalid_pubkey_bytes. */
        for (k = 1; k <= EXHAUSTIVE_TEST_ORDER / 2 + NUM_INVALID_KEYS; ++k) {
            unsigned char sig64[64];
            int actual_k = -1;
            int e_done[EXHAUSTIVE_TEST_ORDER] = {0};
            int e_count_done = 0;
            if (skip_section(&iter)) continue;
            if (k <= EXHAUSTIVE_TEST_ORDER / 2) {
                memcpy(sig64, xonly_pubkey_bytes[k - 1], 32);
                actual_k = parities[k - 1] ? EXHAUSTIVE_TEST_ORDER - k : k;
            } else {
                memcpy(sig64, invalid_pubkey_bytes[k - 1 - EXHAUSTIVE_TEST_ORDER / 2], 32);
            }
            /* Randomly generate messages until all challenges have been hit. */
            while (e_count_done < EXHAUSTIVE_TEST_ORDER) {
                rustsecp256k1_v0_4_0_scalar e;
                unsigned char msg32[32];
                rustsecp256k1_v0_4_0_testrand256(msg32);
                rustsecp256k1_v0_4_0_schnorrsig_challenge(&e, sig64, msg32, pk32);
                /* Only do work if we hit a challenge we haven't tried before. */
                if (!e_done[e]) {
                    /* Iterate over the possible valid last 32 bytes in the signature.
                       0..order=that s value; order+1=random bytes */
                    int count_valid = 0, s;
                    for (s = 0; s <= EXHAUSTIVE_TEST_ORDER + 1; ++s) {
                        int expect_valid, valid;
                        if (s <= EXHAUSTIVE_TEST_ORDER) {
                            rustsecp256k1_v0_4_0_scalar s_s;
                            rustsecp256k1_v0_4_0_scalar_set_int(&s_s, s);
                            rustsecp256k1_v0_4_0_scalar_get_b32(sig64 + 32, &s_s);
                            expect_valid = actual_k != -1 && s != EXHAUSTIVE_TEST_ORDER &&
                                           (s_s == (actual_k + actual_d * e) % EXHAUSTIVE_TEST_ORDER);
                        } else {
                            rustsecp256k1_v0_4_0_testrand256(sig64 + 32);
                            expect_valid = 0;
                        }
                        valid = rustsecp256k1_v0_4_0_schnorrsig_verify(ctx, sig64, msg32, &pubkeys[d - 1]);
                        CHECK(valid == expect_valid);
                        count_valid += valid;
                    }
                    /* Exactly one s value must verify, unless R is illegal. */
                    CHECK(count_valid == (actual_k != -1));
                    /* Don't retry other messages that result in the same challenge. */
                    e_done[e] = 1;
                    ++e_count_done;
                }
            }
        }
    }
}

static void test_exhaustive_schnorrsig_sign(const rustsecp256k1_v0_4_0_context *ctx, unsigned char (*xonly_pubkey_bytes)[32], const rustsecp256k1_v0_4_0_keypair* keypairs, const int* parities) {
    int d, k;
    uint64_t iter = 0;
    /* Loop over keys. */
    for (d = 1; d < EXHAUSTIVE_TEST_ORDER; ++d) {
        int actual_d = d;
        if (parities[d - 1]) actual_d = EXHAUSTIVE_TEST_ORDER - d;
        /* Loop over nonces. */
        for (k = 1; k < EXHAUSTIVE_TEST_ORDER; ++k) {
            int e_done[EXHAUSTIVE_TEST_ORDER] = {0};
            int e_count_done = 0;
            unsigned char msg32[32];
            unsigned char sig64[64];
            int actual_k = k;
            if (skip_section(&iter)) continue;
            if (parities[k - 1]) actual_k = EXHAUSTIVE_TEST_ORDER - k;
            /* Generate random messages until all challenges have been tried. */
            while (e_count_done < EXHAUSTIVE_TEST_ORDER) {
                rustsecp256k1_v0_4_0_scalar e;
                rustsecp256k1_v0_4_0_testrand256(msg32);
                rustsecp256k1_v0_4_0_schnorrsig_challenge(&e, xonly_pubkey_bytes[k - 1], msg32, xonly_pubkey_bytes[d - 1]);
                /* Only do work if we hit a challenge we haven't tried before. */
                if (!e_done[e]) {
                    rustsecp256k1_v0_4_0_scalar expected_s = (actual_k + e * actual_d) % EXHAUSTIVE_TEST_ORDER;
                    unsigned char expected_s_bytes[32];
                    rustsecp256k1_v0_4_0_scalar_get_b32(expected_s_bytes, &expected_s);
                    /* Invoke the real function to construct a signature. */
                    CHECK(rustsecp256k1_v0_4_0_schnorrsig_sign(ctx, sig64, msg32, &keypairs[d - 1], rustsecp256k1_v0_4_0_hardened_nonce_function_smallint, &k));
                    /* The first 32 bytes must match the xonly pubkey for the specified k. */
                    CHECK(rustsecp256k1_v0_4_0_memcmp_var(sig64, xonly_pubkey_bytes[k - 1], 32) == 0);
                    /* The last 32 bytes must match the expected s value. */
                    CHECK(rustsecp256k1_v0_4_0_memcmp_var(sig64 + 32, expected_s_bytes, 32) == 0);
                    /* Don't retry other messages that result in the same challenge. */
                    e_done[e] = 1;
                    ++e_count_done;
                }
            }
        }
    }
}

static void test_exhaustive_schnorrsig(const rustsecp256k1_v0_4_0_context *ctx) {
    rustsecp256k1_v0_4_0_keypair keypair[EXHAUSTIVE_TEST_ORDER - 1];
    rustsecp256k1_v0_4_0_xonly_pubkey xonly_pubkey[EXHAUSTIVE_TEST_ORDER - 1];
    int parity[EXHAUSTIVE_TEST_ORDER - 1];
    unsigned char xonly_pubkey_bytes[EXHAUSTIVE_TEST_ORDER - 1][32];
    unsigned i;

    /* Verify that all invalid_pubkey_bytes are actually invalid. */
    for (i = 0; i < NUM_INVALID_KEYS; ++i) {
        rustsecp256k1_v0_4_0_xonly_pubkey pk;
        CHECK(!rustsecp256k1_v0_4_0_xonly_pubkey_parse(ctx, &pk, invalid_pubkey_bytes[i]));
    }

    /* Construct keypairs and xonly-pubkeys for the entire group. */
    for (i = 1; i < EXHAUSTIVE_TEST_ORDER; ++i) {
        rustsecp256k1_v0_4_0_scalar scalar_i;
        unsigned char buf[32];
        rustsecp256k1_v0_4_0_scalar_set_int(&scalar_i, i);
        rustsecp256k1_v0_4_0_scalar_get_b32(buf, &scalar_i);
        CHECK(rustsecp256k1_v0_4_0_keypair_create(ctx, &keypair[i - 1], buf));
        CHECK(rustsecp256k1_v0_4_0_keypair_xonly_pub(ctx, &xonly_pubkey[i - 1], &parity[i - 1], &keypair[i - 1]));
        CHECK(rustsecp256k1_v0_4_0_xonly_pubkey_serialize(ctx, xonly_pubkey_bytes[i - 1], &xonly_pubkey[i - 1]));
    }

    test_exhaustive_schnorrsig_sign(ctx, xonly_pubkey_bytes, keypair, parity);
    test_exhaustive_schnorrsig_verify(ctx, xonly_pubkey, xonly_pubkey_bytes, parity);
}

#endif

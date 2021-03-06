/*
 * Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
 *
 * Licensed under the Apache License, Version 2.0 (the "License").
 * You may not use this file except in compliance with the License.
 * A copy of the License is located at
 *
 *  http://aws.amazon.com/apache2.0
 *
 * or in the "license" file accompanying this file. This file is distributed
 * on an "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either
 * express or implied. See the License for the specific language governing
 * permissions and limitations under the License.
 */

/* Target Functions: s2n_kem_decapsulate BIKE1_L1_R1_crypto_kem_dec */

#include "tests/s2n_test.h"
#include "tests/testlib/s2n_nist_kats.h"
#include "tests/testlib/s2n_testlib.h"
#include "tls/s2n_kem.h"
#include "utils/s2n_safety.h"
#include "utils/s2n_mem.h"
#include "utils/s2n_blob.h"

#define RSP_FILE_NAME "../unit/kats/kyber_r2.kat"

/* This fuzz test uses the first private key from tests/unit/kats/kyber_r2.kat, the valid ciphertext generated with the
 * public key was copied to corpus/s2n_kyber_r2_fuzz_test/valid_ciphertext */

static struct s2n_kem_params server_kem_params = {.kem = &s2n_kyber_512_r2};

static void s2n_fuzz_atexit()
{
    s2n_kem_free(&server_kem_params);
    s2n_cleanup();
}

int LLVMFuzzerInitialize(const uint8_t *buf, size_t len)
{
    GUARD(s2n_init());
    GUARD_POSIX_STRICT(atexit(s2n_fuzz_atexit));

    GUARD(s2n_alloc(&server_kem_params.private_key, s2n_kyber_512_r2.private_key_length));

    FILE *kat_file = fopen(RSP_FILE_NAME, "r");
    notnull_check(kat_file);
    GUARD(ReadHex(kat_file, server_kem_params.private_key.data, s2n_kyber_512_r2.private_key_length, "sk = "));

    fclose(kat_file);

    return 0;
}

int LLVMFuzzerTestOneInput(const uint8_t *buf, size_t len)
{
    struct s2n_blob ciphertext = {0};
    GUARD(s2n_alloc(&ciphertext, len));

    /* Need to memcpy since blobs expect a non-const value and LLVMFuzzer does expect a const */
    memcpy_check(ciphertext.data, buf, len);

    /* Run the test, don't use GUARD since the memory needs to be cleaned up and decapsulate will most likely fail */
    s2n_kem_decapsulate(&server_kem_params, &ciphertext);

    GUARD(s2n_free(&ciphertext));

    /* The above s2n_kem_decapsulate could fail before ever allocating the server_shared_secret */
    if (server_kem_params.shared_secret.allocated) {
        GUARD(s2n_free(&(server_kem_params.shared_secret)));
    }
    return 0;
}

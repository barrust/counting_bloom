#include <stdlib.h>

#include "minunit.h"
#include "../src/counting_bloom.h"


CountingBloom cb;

void test_setup(void) {
    counting_bloom_init(&cb, 50000, 0.01);
}

void test_teardown(void) {
    counting_bloom_destroy(&cb);
}


/*******************************************************************************
*   Test setup
*******************************************************************************/
MU_TEST(test_bloom_setup) {
    mu_assert_int_eq(50000, cb.estimated_elements);
    float fpr = 0.01;
    mu_assert_double_eq(fpr, cb.false_positive_probability);
    mu_assert_int_eq(7, cb.number_hashes);
    mu_assert_int_eq(479253, cb.number_bits);
    mu_assert_int_eq(0, cb.elements_added);

    mu_assert_null(cb.filepointer);
    mu_assert_not_null(cb.hash_function);
}

MU_TEST(test_bloom_setup_returns) {
    CountingBloom bf;
    mu_assert_int_eq(COUNTING_BLOOM_FAILURE, counting_bloom_init(&bf, 0, 0.01));
    mu_assert_int_eq(COUNTING_BLOOM_FAILURE, counting_bloom_init(&bf, 50000, 1.01));
    mu_assert_int_eq(COUNTING_BLOOM_FAILURE, counting_bloom_init(&bf, 50000, -0.01));
    mu_assert_int_eq(COUNTING_BLOOM_SUCCESS, counting_bloom_init(&bf, 50000, 0.01));
    counting_bloom_destroy(&bf);
}

MU_TEST(test_bloom_on_disk_setup) {
    char filepath[] = "./dist/test_bloom_on_disk_setup.blm";

    CountingBloom bf;
    int r = counting_bloom_init_on_disk(&bf, 50000, 0.01, filepath);

    mu_assert_int_eq(COUNTING_BLOOM_SUCCESS, r);
    mu_assert_int_eq(50000, bf.estimated_elements);
    float fpr = 0.01;
    mu_assert_double_eq(fpr, bf.false_positive_probability);
    mu_assert_int_eq(7, bf.number_hashes);
    mu_assert_int_eq(479253, cb.number_bits);
    mu_assert_int_eq(0, bf.elements_added);

    mu_assert_not_null(bf.filepointer);
    mu_assert_not_null(bf.hash_function);

    counting_bloom_destroy(&bf);
    remove(filepath);
}

MU_TEST(test_bloom_on_disk_setup_returns) {
    char filepath[] = "./dist/test_bloom_on_disk_setup_returns.blm";
    CountingBloom bf;
    mu_assert_int_eq(COUNTING_BLOOM_FAILURE, counting_bloom_init_on_disk(&bf, 0, 0.01, filepath));
    mu_assert_int_eq(COUNTING_BLOOM_FAILURE, counting_bloom_init_on_disk(&bf, 50000, 1.01, filepath));
    mu_assert_int_eq(COUNTING_BLOOM_FAILURE, counting_bloom_init_on_disk(&bf, 50000, -0.01, filepath));
    mu_assert_int_eq(COUNTING_BLOOM_SUCCESS, counting_bloom_init_on_disk(&bf, 50000, 0.01, filepath));
    counting_bloom_destroy(&bf);
    remove(filepath);
}

/*******************************************************************************
*   Test set and check
*******************************************************************************/
MU_TEST(test_bloom_set) {
    int errors = 0;
    for (int i = 0; i < 3000; ++i) {
        char key[10] = {0};
        sprintf(key, "%d", i);
        errors += counting_bloom_add_string(&cb, key) == COUNTING_BLOOM_SUCCESS ? 0 : 1;
    }
    mu_assert_int_eq(0, errors);
    mu_assert_int_eq(3000, cb.elements_added);

    errors = 0;
    for (int i = 0; i < 3000; ++i) {
        char key[10] = {0};
        sprintf(key, "%d", i);
        errors += counting_bloom_check_string(&cb, key) == COUNTING_BLOOM_SUCCESS ? 0 : 1;
    }
    mu_assert_int_eq(0, errors);
}

MU_TEST(test_bloom_set_failure) {
    uint64_t* hashes = counting_bloom_calculate_hashes(&cb, "three", 3); // we want too few!
    mu_assert_int_eq(COUNTING_BLOOM_FAILURE, counting_bloom_add_string_alt(&cb, hashes, 3));
    free(hashes);
}

MU_TEST(test_bloom_set_on_disk) {
    /*  set on disk is different than in memory because some values have to be
        updated on disk; so this must be tested seperately */
    char filepath[] = "./dist/test_bloom_set_on_disk.blm";

    CountingBloom bf;
    counting_bloom_init_on_disk(&bf, 50000, 0.01, filepath);

    int errors = 0;
    for (int i = 0; i < 3000; ++i) {
        char key[10] = {0};
        sprintf(key, "%d", i);
        errors += counting_bloom_add_string(&bf, key) == COUNTING_BLOOM_SUCCESS ? 0 : 1;
    }
    mu_assert_int_eq(0, errors);
    mu_assert_int_eq(3000, bf.elements_added);

    errors = 0;
    for (int i = 0; i < 3000; ++i) {
        char key[10] = {0};
        sprintf(key, "%d", i);
        errors += counting_bloom_check_string(&bf, key) == COUNTING_BLOOM_SUCCESS ? 0 : 1;
    }
    mu_assert_int_eq(0, errors);
    counting_bloom_destroy(&bf);
    remove(filepath);
}

MU_TEST(test_bloom_check) {
    int errors = 0;
    for (int i = 0; i < 3000; ++i) {
        char key[10] = {0};
        sprintf(key, "%d", i);
        errors += counting_bloom_add_string(&cb, key) == COUNTING_BLOOM_SUCCESS ? 0 : 1;
    }

    /* check things that are not present */
    errors = 0;
    for (int i = 3000; i < 5000; ++i) {
        char key[10] = {0};
        sprintf(key, "%d", i);
        errors += counting_bloom_check_string(&cb, key) == COUNTING_BLOOM_FAILURE ? 0 : 1;
    }
    mu_assert_int_eq(0, errors);
}

MU_TEST(test_bloom_check_false_positive) {
    int errors = 0;
    for (int i = 0; i < 50000; ++i) {
        char key[10] = {0};
        sprintf(key, "%d", i);
        errors += counting_bloom_add_string(&cb, key) == COUNTING_BLOOM_SUCCESS ? 0 : 1;
    }

    /* check things that are not present */
    errors = 0;
    for (int i = 50000; i < 51000; ++i) {
        char key[10] = {0};
        sprintf(key, "%d", i);
        errors += counting_bloom_check_string(&cb, key) == COUNTING_BLOOM_FAILURE ? 0 : 1;
    }
    mu_assert_int_eq(11, errors);  // there are 11 false positives!
}

MU_TEST(test_bloom_check_failure) {
    uint64_t* hashes = counting_bloom_calculate_hashes(&cb, "three", 3); // we want too few!
    mu_assert_int_eq(COUNTING_BLOOM_FAILURE, counting_bloom_check_string_alt(&cb, hashes, 3));
    free(hashes);
}


/*******************************************************************************
*   Testsuite
*******************************************************************************/
MU_TEST_SUITE(test_suite) {
    MU_SUITE_CONFIGURE(&test_setup, &test_teardown);

    /* setup */
    MU_RUN_TEST(test_bloom_setup);
    MU_RUN_TEST(test_bloom_setup_returns);
    MU_RUN_TEST(test_bloom_on_disk_setup);
    MU_RUN_TEST(test_bloom_on_disk_setup_returns);

    /* set and contains */
    MU_RUN_TEST(test_bloom_set);
    MU_RUN_TEST(test_bloom_set_failure);
    MU_RUN_TEST(test_bloom_set_on_disk);
    MU_RUN_TEST(test_bloom_check);
    MU_RUN_TEST(test_bloom_check_false_positive);
    MU_RUN_TEST(test_bloom_check_failure);
}


int main() {
    // we want to ignore stderr print statements
    freopen("/dev/null", "w", stderr);

    MU_RUN_SUITE(test_suite);
    MU_REPORT();
    printf("Number failed tests: %d\n", minunit_fail);
    return minunit_fail;
}

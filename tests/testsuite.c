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
*   Testsuite
*******************************************************************************/
MU_TEST_SUITE(test_suite) {
    MU_SUITE_CONFIGURE(&test_setup, &test_teardown);

    /* setup */
    MU_RUN_TEST(test_bloom_setup);
    MU_RUN_TEST(test_bloom_setup_returns);
    MU_RUN_TEST(test_bloom_on_disk_setup);
    MU_RUN_TEST(test_bloom_on_disk_setup_returns);
}


int main() {
    // we want to ignore stderr print statements
    freopen("/dev/null", "w", stderr);

    MU_RUN_SUITE(test_suite);
    MU_REPORT();
    printf("Number failed tests: %d\n", minunit_fail);
    return minunit_fail;
}

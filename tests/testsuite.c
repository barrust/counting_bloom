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

MU_TEST(test_bloom_get_max_insertions) {
    int errors = 0;
    for (int i = 0; i < 3000; ++i) {
        char key[10] = {0};
        sprintf(key, "%d", i);
        errors += counting_bloom_add_string(&cb, key) == COUNTING_BLOOM_SUCCESS ? 0 : 1;
        if (i % 2 == 0)
            errors += counting_bloom_add_string(&cb, key) == COUNTING_BLOOM_SUCCESS ? 0 : 1;
    }
    mu_assert_int_eq(0, errors);
    mu_assert_int_eq(4500, cb.elements_added);

    errors = 0;
    for (int i = 0; i < 3000; ++i) {
        char key[10] = {0};
        sprintf(key, "%d", i);
        int res = counting_bloom_get_max_insertions(&cb, key);
        if (i % 2 == 0)
            errors += res == 2 ? 0 : 1;  // should be there twice!
        else
            errors += res == 1 ? 0 : 1;  // should be there once!
    }
    mu_assert_int_eq(0, errors);
}

/*******************************************************************************
*   Test clear/reset
*******************************************************************************/
// MU_TEST(test_bloom_clear) {
//     for (int i = 0; i < 5000; ++i) {
//         char key[10] = {0};
//         sprintf(key, "%d", i);
//         counting_bloom_add_string(&cb, key);
//     }
//     mu_assert_int_eq(5000, cb.elements_added);
//
//     mu_assert_int_eq(COUNTING_BLOOM_SUCCESS, counting_bloom_clear(&b));
//
//     mu_assert_int_eq(0, b.elements_added);
//
//     int errors = 0;
//     for (int i = 0; i < 5000; ++i) {
//         char key[10] = {0};
//         sprintf(key, "%d", i);
//         errors += counting_bloom_check_string(&cb, key) == COUNTING_BLOOM_SUCCESS ? 0 : 1;
//     }
//     mu_assert_int_eq(5000, errors);
// }

/*******************************************************************************
*   Test statistics
*******************************************************************************/
MU_TEST(test_bloom_current_false_positive_rate) {
    mu_assert_double_eq(0.00000, counting_bloom_current_false_positive_rate(&cb));

    for (int i = 0; i < 5000; ++i) {
        char key[10] = {0};
        sprintf(key, "%d", i);
        counting_bloom_add_string(&cb, key);
    }

    mu_assert_double_between(0.0000, 0.0010, counting_bloom_current_false_positive_rate(&cb));

    for (int i = 5000; i < 50000; ++i) {
        char key[10] = {0};
        sprintf(key, "%d", i);
        counting_bloom_add_string(&cb, key);
    }

    mu_assert_double_between(0.00990, 0.01010, counting_bloom_current_false_positive_rate(&cb));
}

// MU_TEST(test_bloom_count_set_bits) {
//     mu_assert_int_eq(0, bloom_filter_count_set_bits(&b));
//
//     bloom_filter_add_string(&b, "a");
//     mu_assert_int_eq(b.number_hashes, bloom_filter_count_set_bits(&b));
//
//     /* add a few keys */
//     for (int i = 0; i < 5000; ++i) {
//         char key[10] = {0};
//         sprintf(key, "%d", i);
//         bloom_filter_add_string(&b, key);
//     }
//     mu_assert_int_eq(33592, bloom_filter_count_set_bits(&b));
// }

// MU_TEST(test_bloom_export_size) {  // size is in bytes
//     mu_assert_int_eq(59927, counting_bloom_export_size(&cb));
//
//     CountingBloom bf;
//     counting_bloom_init(&bf, 100000, .5);
//     mu_assert_int_eq(18054, counting_bloom_export_size(&bf));
//     counting_bloom_destroy(&bf);
//
//     counting_bloom_init(&bf, 100000, .1);
//     mu_assert_int_eq(59927, counting_bloom_export_size(&bf));
//     counting_bloom_destroy(&bf);
//
//     counting_bloom_init(&bf, 100000, .05);
//     mu_assert_int_eq(77961, counting_bloom_export_size(&bf));
//     counting_bloom_destroy(&bf);
//
//     counting_bloom_init(&bf, 100000, .01);
//     mu_assert_int_eq(119834, counting_bloom_export_size(&bf));
//     counting_bloom_destroy(&bf);
//
//     counting_bloom_init(&bf, 100000, .001);
//     mu_assert_int_eq(179740, counting_bloom_export_size(&bf));
//     counting_bloom_destroy(&bf);
// }

// MU_TEST(test_bloom_estimate_elements) {
//     for (int i = 0; i < 5000; ++i) {
//         char key[10] = {0};
//         sprintf(key, "%d", i);
//         counting_bloom_add_string(&cb, key);
//     }
//     mu_assert_int_eq(5000, b.elements_added);
//     mu_assert_int_eq(4974, counting_bloom_estimate_elements(&cb));
//
//     for (int i = 5000; i < 10000; ++i) {
//         char key[10] = {0};
//         sprintf(key, "%d", i);
//         counting_bloom_add_string(&cb, key);
//     }
//     mu_assert_int_eq(10000, b.elements_added);
//     mu_assert_int_eq(9960, counting_bloom_estimate_elements(&cb));
// }

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
    MU_RUN_TEST(test_bloom_get_max_insertions);

    /* clear, reset */
    // MU_RUN_TEST(test_bloom_clear);

    /* statistics */
    MU_RUN_TEST(test_bloom_current_false_positive_rate);
    // MU_RUN_TEST(test_bloom_count_set_bits);
    // MU_RUN_TEST(test_bloom_export_size);
    // MU_RUN_TEST(test_bloom_estimate_elements);
}


int main() {
    // we want to ignore stderr print statements
    freopen("/dev/null", "w", stderr);

    MU_RUN_SUITE(test_suite);
    MU_REPORT();
    printf("Number failed tests: %d\n", minunit_fail);
    return minunit_fail;
}

#include <stdlib.h>
#include <sys/stat.h>
#include <sys/types.h>

#include <openssl/md5.h>

#include "minunit.h"
#include "../src/counting_bloom.h"


static int calculate_md5sum(const char* filename, char* digest);
static off_t fsize(const char* filename);

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

    // make sure elements added is correct!
    counting_bloom_import(&bf, filepath);
    mu_assert_int_eq(3000, bf.elements_added);
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

MU_TEST(test_bloom_get_max_insertions_missing) {
    int errors = 0;
    for (int i = 0; i < 1500; ++i) {
        char key[10] = {0};
        sprintf(key, "%d", i);
        errors += counting_bloom_add_string(&cb, key) == COUNTING_BLOOM_SUCCESS ? 0 : 1;
        errors += counting_bloom_add_string(&cb, key) == COUNTING_BLOOM_SUCCESS ? 0 : 1;
    }
    mu_assert_int_eq(0, errors);
    mu_assert_int_eq(3000, cb.elements_added);

    for (int i = 0; i < 3000; ++i) {
        char key[10] = {0};
        sprintf(key, "%d", i);

        int res = counting_bloom_get_max_insertions(&cb, key);

        if (i < 1500) {
            errors += res == 2 ? 0 : 1;  // check we got the insertion
        } else {
            errors += res == 0 ? 0 : 1;  // check we got nothing returned
        }
    }

    mu_assert_int_eq(0, errors);
}

MU_TEST(test_bloom_remove) {
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
    for (int i = 0; i < 1500; i+=2) {
        char key[10] = {0};
        sprintf(key, "%d", i);
        int res = counting_bloom_get_max_insertions(&cb, key);
        errors += res == 2 ? 0 : 1;  // should be there twice!

        res = counting_bloom_remove_string(&cb, key);
        errors += res == COUNTING_BLOOM_SUCCESS ? 0 : 1;  // check we got success

        res = counting_bloom_get_max_insertions(&cb, key);
        errors += res == 1 ? 0 : 1;  // should be there once!
    }
    mu_assert_int_eq(0, errors);
}

MU_TEST(test_bloom_remove_fail) {
    int errors = 0;
    for (int i = 0; i < 1500; ++i) {
        char key[10] = {0};
        sprintf(key, "%d", i);
        errors += counting_bloom_add_string(&cb, key) == COUNTING_BLOOM_SUCCESS ? 0 : 1;
    }
    mu_assert_int_eq(0, errors);
    mu_assert_int_eq(1500, cb.elements_added);

    for (int i = 0; i < 3000; ++i) {
        char key[10] = {0};
        sprintf(key, "%d", i);

        int res = counting_bloom_remove_string(&cb, key);

        if (i < 1500) {
            errors += res == COUNTING_BLOOM_SUCCESS ? 0 : 1;  // check we got success
        } else {
            errors += res == COUNTING_BLOOM_FAILURE ? 0 : 1;  // check we got success
        }
    }

    mu_assert_int_eq(0, errors);
    mu_assert_int_eq(0, cb.elements_added);
}

MU_TEST(test_bloom_remove_on_disk_with_failures) {
    /*  set on disk is different than in memory because some values have to be
        updated on disk; so this must be tested seperately */
    char filepath[] = "./dist/test_bloom_set_on_disk.blm";
    int errors = 0;
    CountingBloom bf;
    counting_bloom_init_on_disk(&bf, 50000, 0.01, filepath);

    for (int i = 0; i < 1500; ++i) {
        char key[10] = {0};
        sprintf(key, "%d", i);
        errors += counting_bloom_add_string(&bf, key) == COUNTING_BLOOM_SUCCESS ? 0 : 1;
    }
    mu_assert_int_eq(0, errors);
    mu_assert_int_eq(1500, bf.elements_added);

    for (int i = 0; i < 3000; ++i) {
        char key[10] = {0};
        sprintf(key, "%d", i);

        int res = counting_bloom_remove_string(&bf, key);

        if (i < 1500) {
            errors += res == COUNTING_BLOOM_SUCCESS ? 0 : 1;  // check we got success
        } else {
            errors += res == COUNTING_BLOOM_FAILURE ? 0 : 1;  // check we got success
        }
    }

    mu_assert_int_eq(0, errors);
    mu_assert_int_eq(0, bf.elements_added);

    counting_bloom_destroy(&bf);

    // re-import the counting bloom filter to see if elements added was correctly set!
    counting_bloom_import(&bf, filepath);
    mu_assert_int_eq(0, bf.elements_added);
    counting_bloom_destroy(&bf);

    remove(filepath);
}

/*******************************************************************************
*   Test clear/reset
*******************************************************************************/
MU_TEST(test_bloom_clear) {
    for (int i = 0; i < 5000; ++i) {
        char key[10] = {0};
        sprintf(key, "%d", i);
        counting_bloom_add_string(&cb, key);
    }
    mu_assert_int_eq(5000, cb.elements_added);

    mu_assert_int_eq(COUNTING_BLOOM_SUCCESS, counting_bloom_clear(&cb));

    mu_assert_int_eq(0, cb.elements_added);

    int errors = 0;
    for (int i = 0; i < 5000; ++i) {
        char key[10] = {0};
        sprintf(key, "%d", i);
        errors += counting_bloom_check_string(&cb, key) == COUNTING_BLOOM_SUCCESS ? 0 : 1;
    }
    mu_assert_int_eq(5000, errors);
}

MU_TEST(test_bloom_clear_on_disk) {
    char filepath[] = "./dist/test_bloom_set_on_disk.blm";
    CountingBloom bf;
    counting_bloom_init_on_disk(&bf, 50000, 0.01, filepath);
    for (int i = 0; i < 5000; ++i) {
        char key[10] = {0};
        sprintf(key, "%d", i);
        counting_bloom_add_string(&bf, key);
    }
    mu_assert_int_eq(5000, bf.elements_added);

    mu_assert_int_eq(COUNTING_BLOOM_SUCCESS, counting_bloom_clear(&bf));

    mu_assert_int_eq(0, bf.elements_added);

    int errors = 0;
    for (int i = 0; i < 5000; ++i) {
        char key[10] = {0};
        sprintf(key, "%d", i);
        errors += counting_bloom_check_string(&bf, key) == COUNTING_BLOOM_SUCCESS ? 0 : 1;
    }
    mu_assert_int_eq(5000, errors);

    counting_bloom_destroy(&bf);

    // re-import the counting bloom filter to see if elements added was correctly set!
    counting_bloom_import(&bf, filepath);
    mu_assert_int_eq(0, bf.elements_added);
    counting_bloom_destroy(&bf);

    remove(filepath);
}

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

MU_TEST(test_bloom_count_set_bits) {
    mu_assert_int_eq(0, counting_bloom_count_set_bits(&cb));

    counting_bloom_add_string(&cb, "a");
    mu_assert_int_eq(cb.number_hashes, counting_bloom_count_set_bits(&cb));

    /* add a few keys */
    for (int i = 0; i < 5000; ++i) {
        char key[10] = {0};
        sprintf(key, "%d", i);
        counting_bloom_add_string(&cb, key);
    }
    mu_assert_int_eq(33592, counting_bloom_count_set_bits(&cb));
}

MU_TEST(test_bloom_export_size) {  // size is in bytes
    mu_assert_int_eq(1917024, counting_bloom_export_size(&cb));

    CountingBloom bf;
    counting_bloom_init(&bf, 100000, .5);
    mu_assert_int_eq(577092, counting_bloom_export_size(&bf));
    counting_bloom_destroy(&bf);

    counting_bloom_init(&bf, 100000, .1);
    mu_assert_int_eq(1917024, counting_bloom_export_size(&bf));
    counting_bloom_destroy(&bf);

    counting_bloom_init(&bf, 100000, .05);
    mu_assert_int_eq(2494104, counting_bloom_export_size(&bf));
    counting_bloom_destroy(&bf);

    counting_bloom_init(&bf, 100000, .01);
    mu_assert_int_eq(3834036, counting_bloom_export_size(&bf));
    counting_bloom_destroy(&bf);

    counting_bloom_init(&bf, 100000, .001);
    mu_assert_int_eq(5751048, counting_bloom_export_size(&bf));
    counting_bloom_destroy(&bf);
}

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
*   Test Import / Export
*******************************************************************************/
MU_TEST(test_bloom_export) {
    char filepath[] = "./dist/test_bloom_export.blm";
    for (int i = 0; i < 5000; ++i) {
        char key[10] = {0};
        sprintf(key, "%d", i);
        counting_bloom_add_string(&cb, key);
    }

    mu_assert_int_eq(COUNTING_BLOOM_SUCCESS, counting_bloom_export(&cb, filepath));

    char digest[33] = {0};
    calculate_md5sum(filepath, digest);
    mu_assert_string_eq("6e434129d9d0238bbb650ff800abfac3", digest);
    mu_assert_int_eq(1917032, fsize(filepath));
    remove(filepath);
}

MU_TEST(test_bloom_export_on_disk) {
    // exporting on disk just closes the file!
    char filepath[] = "./dist/test_bloom_export_on_disk.blm";

    CountingBloom bf;
    counting_bloom_init_on_disk(&bf, 50000, 0.01, filepath);

    for (int i = 0; i < 5000; ++i) {
        char key[10] = {0};
        sprintf(key, "%d", i);
        counting_bloom_add_string(&bf, key);
    }

    mu_assert_int_eq(COUNTING_BLOOM_SUCCESS, counting_bloom_export(&bf, filepath));

    counting_bloom_destroy(&bf);

    char digest[33] = {0};
    calculate_md5sum(filepath, digest);
    mu_assert_string_eq("6e434129d9d0238bbb650ff800abfac3", digest);
    mu_assert_int_eq(1917032, fsize(filepath));
    remove(filepath);
}

MU_TEST(test_bloom_import) {
    char filepath[] = "./dist/test_bloom_import.blm";
    for (int i = 0; i < 5000; ++i) {
        char key[10] = {0};
        sprintf(key, "%d", i);
        counting_bloom_add_string(&cb, key);
    }

    mu_assert_int_eq(COUNTING_BLOOM_SUCCESS, counting_bloom_export(&cb, filepath));

    // now load the file back in!
    CountingBloom bf;
    counting_bloom_import(&bf, filepath);

    mu_assert_int_eq(50000, bf.estimated_elements);
    float fpr = 0.01;
    mu_assert_double_eq(fpr, bf.false_positive_probability);
    mu_assert_int_eq(7, bf.number_hashes);
    mu_assert_int_eq(479253, bf.number_bits);
    mu_assert_int_eq(5000, bf.elements_added);
    int errors = 0;
    for (int i = 0; i < 5000; ++i) {
        char key[10] = {0};
        sprintf(key, "%d", i);
        errors += counting_bloom_check_string(&bf, key) == COUNTING_BLOOM_SUCCESS ? 0 : 1;
    }
    mu_assert_int_eq(0, errors);
    counting_bloom_destroy(&bf);
    remove(filepath);
}

/* NOTE: apparently import does not check all possible failures! */
MU_TEST(test_bloom_import_fail) {
    char filepath[] = "./dist/test_bloom_import_fail.blm";
    CountingBloom bf;
    mu_assert_int_eq(COUNTING_BLOOM_FAILURE, counting_bloom_import(&bf, filepath));
}

MU_TEST(test_bloom_import_on_disk) {
    char filepath[] = "./dist/test_bloom_import.blm";
    for (int i = 0; i < 5000; ++i) {
        char key[10] = {0};
        sprintf(key, "%d", i);
        counting_bloom_add_string(&cb, key);
    }

    mu_assert_int_eq(COUNTING_BLOOM_SUCCESS, counting_bloom_export(&cb, filepath));

    // now load the file back in!
    CountingBloom bf;
    counting_bloom_import_on_disk(&bf, filepath);

    mu_assert_int_eq(50000, bf.estimated_elements);
    float fpr = 0.01;
    mu_assert_double_eq(fpr, bf.false_positive_probability);
    mu_assert_int_eq(7, bf.number_hashes);
    mu_assert_int_eq(479253, bf.number_bits);  // same as bloom_length
    mu_assert_int_eq(5000, bf.elements_added);
    int errors = 0;
    for (int i = 0; i < 5000; ++i) {
        char key[10] = {0};
        sprintf(key, "%d", i);
        errors += counting_bloom_check_string(&bf, key) == COUNTING_BLOOM_SUCCESS ? 0 : 1;
    }
    mu_assert_int_eq(0, errors);
    counting_bloom_destroy(&bf);
    remove(filepath);
}

MU_TEST(test_bloom_import_on_disk_fail) {
    char filepath[] = "./dist/test_bloom_import_on_disk_fail.blm";
    CountingBloom bf;
    mu_assert_int_eq(COUNTING_BLOOM_FAILURE, counting_bloom_import_on_disk(&bf, filepath));
}

/*******************************************************************************
*   Test Statistics
*******************************************************************************/
MU_TEST(test_bloom_filter_stat) {
    for (int i = 0; i < 400; ++i) {
        char key[10] = {0};
        sprintf(key, "%d", i);
        counting_bloom_add_string(&cb, key);
    }

    /* save the printout to a buffer */
    int stdout_save;
    char buffer[2046] = {0};
    fflush(stdout); //clean everything first
    stdout_save = dup(STDOUT_FILENO); //save the stdout state
    freopen("output_file", "a", stdout); //redirect stdout to null pointer
    setvbuf(stdout, buffer, _IOFBF, 1024); //set buffer to stdout

    counting_bloom_stats(&cb);

    /* reset stdout */
    freopen("output_file", "a", stdout); //redirect stdout to null again
    dup2(stdout_save, STDOUT_FILENO); //restore the previous state of stdout
    setvbuf(stdout, NULL, _IONBF, 0); //disable buffer to print to screen instantly

    // Not sure this is necessary, but it cleans it up
    remove("output_file");

    mu_assert_not_null(buffer);
    mu_assert_string_eq("CountingBloom\n\
    bits: 479253\n\
    estimated elements: 50000\n\
    number hashes: 7\n\
    max false positive rate: 0.010000\n\
    elements added: 400\n\
    current false positive rate: 0.000000\n\
    is on disk: no\n\
    index fullness: 0.005822\n\
    max index usage: 2\n\
    max index id: 7890\n\
    calculated elements: 400\n", buffer);
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
    MU_RUN_TEST(test_bloom_get_max_insertions);
    MU_RUN_TEST(test_bloom_get_max_insertions_missing);
    MU_RUN_TEST(test_bloom_remove);
    MU_RUN_TEST(test_bloom_remove_fail);
    MU_RUN_TEST(test_bloom_remove_on_disk_with_failures);
    // MU_RUN_TEST(test_bloom_overflow); // How? currently can't add a lot at once


    /* clear, reset */
    MU_RUN_TEST(test_bloom_clear);
    MU_RUN_TEST(test_bloom_clear_on_disk);

    /* statistics */
    MU_RUN_TEST(test_bloom_current_false_positive_rate);
    MU_RUN_TEST(test_bloom_count_set_bits);
    MU_RUN_TEST(test_bloom_export_size);
    // MU_RUN_TEST(test_bloom_estimate_elements);

    /* export, import */
    MU_RUN_TEST(test_bloom_export);
    MU_RUN_TEST(test_bloom_export_on_disk);
    MU_RUN_TEST(test_bloom_import);
    MU_RUN_TEST(test_bloom_import_fail);
    MU_RUN_TEST(test_bloom_import_on_disk);
    MU_RUN_TEST(test_bloom_import_on_disk_fail);

    /* Statistics */
    MU_RUN_TEST(test_bloom_filter_stat);
}


int main() {
    // we want to ignore stderr print statements
    freopen("/dev/null", "w", stderr);

    MU_RUN_SUITE(test_suite);
    MU_REPORT();
    printf("Number failed tests: %d\n", minunit_fail);
    return minunit_fail;
}




static int calculate_md5sum(const char* filename, char* digest) {
    FILE *file_ptr;
    file_ptr = fopen(filename, "r");
    if (file_ptr == NULL) {
        perror("Error opening file");
        fflush(stdout);
        return 1;
    }

    int n;
    MD5_CTX c;
    char buf[512];
    ssize_t bytes;
    unsigned char out[MD5_DIGEST_LENGTH];

    MD5_Init(&c);
    do {
        bytes = fread(buf, 1, 512, file_ptr);
        MD5_Update(&c, buf, bytes);
    } while(bytes > 0);

    MD5_Final(out, &c);

    for (n = 0; n < MD5_DIGEST_LENGTH; n++) {
        char hex[3] = {0};
        sprintf(hex, "%02x", out[n]);
        digest[n*2] = hex[0];
        digest[n*2+1] = hex[1];
    }

    fclose(file_ptr);

    return 0;
}

static off_t fsize(const char* filename) {
    struct stat st;

    if (stat(filename, &st) == 0)
        return st.st_size;

    return -1;
}

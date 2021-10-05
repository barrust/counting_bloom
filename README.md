# Counting Bloom
[![License: MIT](https://img.shields.io/badge/License-MIT-blue.svg)](https://opensource.org/licenses/MIT)
[![GitHub release](https://img.shields.io/github/v/release/barrust/counting_bloom.svg)](https://github.com/barrust/counting_bloom/releases)
[![Build Status](https://github.com/barrust/counting_bloom/workflows/C/C++%20CI/badge.svg?branch=master)](https://github.com/barrust/counting_bloom/actions)
[![codecov](https://codecov.io/gh/barrust/counting_bloom/branch/master/graph/badge.svg)](https://codecov.io/gh/barrust/counting_bloom)

Counting Bloom Filter implementation written in **C**

A counting bloom filter is similar to a standard bloom filter except that instead
of using bits, one increments each index in the array. This allows for the removal
of items from the filter. The data stored in a Bloom Filter is not retrievable. Once
data is 'inserted', data can be checked to see if it likely has been seen or if
it definitely has not. Bloom Filters guarantee a 0% False Negative rate with a
pre-selected false positive rate.

To use the library, copy the `src/counting_bloom.h` and `src/counting_bloom.c`
files into your project and include it where needed.

For a **python version**, please check out [pyprobables](https://github.com/barrust/pyprobables)
which has a binary compatible output.

## License:
MIT 2015

## Usage:
``` c
#include <stdio.h>
#include "counting_bloom.h"


CountingBloom cb;
counting_bloom_init(&cb, 10, 0.01);
counting_bloom_add_string(&cb, "google");
counting_bloom_add_string(&cb, "facebook");
counting_bloom_add_string(&cb, "twitter");
counting_bloom_add_string(&cb, "google");
if (counting_bloom_check_string(&cb, "google") == COUNTING_BLOOM_SUCCESS) {
	printf("'google' is in the counting bloom a maximum of %d times!\n", counting_bloom_get_max_insertions(&cb, "google"));
}
counting_bloom_remove_string(&cb, "google");
counting_bloom_stats(&cb);
counting_bloom_destroy(&cb);
```

## Required Compile Flags:
-lm


## Backward Compatible Hash Function
To use the older counting bloom filters (v1.0.3 or lower) that utilized the default hashing
algorithm, then change use the following code as the hash function:

``` c
/* NOTE: The caller will free the results */
static uint64_t* original_default_hash(unsigned int num_hashes, const char* str) {
    uint64_t *results = (uint64_t*)calloc(num_hashes, sizeof(uint64_t));
    char key[17] = {0}; // largest value is 7FFF,FFFF,FFFF,FFFF
    results[0] = __fnv_1a(str);
    for (unsigned int i = 1; i < num_hashes; ++i) {
        sprintf(key, "%" PRIx64 "", results[i-1]);
        results[i] = old_fnv_1a(key);
    }
    return results;
}

static uint64_t old_fnv_1a(const char* key) {
    // FNV-1a hash (http://www.isthe.com/chongo/tech/comp/fnv/)
    int i, len = strlen(key);
    uint64_t h = 14695981039346656073ULL; // FNV_OFFSET 64 bit
    for (i = 0; i < len; ++i){
            h = h ^ (unsigned char) key[i];
            h = h * 1099511628211ULL; // FNV_PRIME 64 bit
    }
    return h;
}
```

If using only older Counting Bloom Filters, then you can update the // FNV_OFFSET 64 bit
to use `14695981039346656073ULL`

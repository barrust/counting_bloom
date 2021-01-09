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

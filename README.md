# Counting Bloom
A quick and simple c implementation of a counting bloom filter.

A counting bloom filter is similar to a standard bloom filter except that instead
of using bits, one increments each index in the array. This allows for the removal
of items from the filter.

##License:
MIT 2015

##Usage:
```
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

##Required Compile Flags:
-lm -lcrypto

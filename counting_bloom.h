
/*******************************************************************************
***
***	 Author: Tyler Barrus
***	 email:  barrust@gmail.com
***
***	 Version: 0.5.0
***	 Purpose: Simple, yet effective, counting bloom filter implementation
***
***	 License: MIT 2015
***
***	 URL:
***
***	 Usage:
***
***
***	Required Compile Flags: -lm -lcrypto
***
*******************************************************************************/


#ifndef __COUNTING_BLOOM_FILTER_H__
#define __COUNTING_BLOOM_FILTER_H__

#include <stdlib.h>         /* calloc, malloc */
#include <inttypes.h>       /* PRIu64 */
#include <math.h>           /* pow, exp */
#include <stdio.h>          /* printf */
#include <string.h>         /* strlen */
#include <limits.h>         /* UINT_MAX */
#include <openssl/md5.h>

#define COUNTING_BLOOMFILTER_VERSION "0.5.0"
#define COUNTING_BLOOMFILTER_MAJOR 0
#define COUNTING_BLOOMFILTER_MINOR 5
#define COUNTING_BLOOMFILTER_REVISION 0

#define COUNTING_BLOOM_SUCCESS 0
#define COUNTING_BLOOM_FAILURE -1

#define counting_bloom_get_version()	(COUNTING_BLOOMFILTER_VERSION)

typedef uint64_t* (*HashFunction)       (int num_hashes, char *str);

typedef struct counting_bloom_filter {
	/* bloom parameters */
	uint64_t estimated_elements;
	float false_positive_probability;
	unsigned int number_hashes;
	uint64_t number_bits;
	/* bloom filter */
	unsigned int *bloom;
	uint64_t elements_added;
	HashFunction hash_function;
} CountingBloom;


int counting_bloom_init(CountingBloom *cb, uint64_t estimated_elements, float false_positive_rate, HashFunction hash_function);

/* not implemented */
void counting_bloom_stats(CountingBloom *cb);

int counting_bloom_destroy(CountingBloom *cb);

int counting_bloom_add_string(CountingBloom *cb, char *str);

int counting_bloom_add_string_alt(CountingBloom *cb, uint64_t* hashes, unsigned int number_hashes_passed);

int counting_bloom_check_string(CountingBloom *cb, char *str);

int counting_bloom_check_string_alt(CountingBloom *cb, uint64_t* hashes, unsigned int number_hashes_passed);

int counting_bloom_get_max_insertions(CountingBloom *cb, char *str);

/* not implemented */
int counting_bloom_remove_string(CountingBloom *cb, char *str);

/* not implemented */
int counting_bloom_import_on_disk(CountingBloom *cb, char *filepath, HashFunction hash_function);

/* not implemented */
int counting_bloom_export(CountingBloom *cb, char *filepath);

float counting_bloom_current_false_positive_rate(CountingBloom *cb);

uint64_t* counting_bloom_calculate_hashes(CountingBloom *cb, char *str, unsigned int number_hashes);

#endif /* END COUNTING BLOOM FILTER HEADER */

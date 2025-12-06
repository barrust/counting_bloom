#ifndef BARRUST_COUNTING_BLOOM_FILTER_H__
#define BARRUST_COUNTING_BLOOM_FILTER_H__
/*******************************************************************************
***
***	 Author: Tyler Barrus
***	 email:  barrust@gmail.com
***
***	 Version: 1.1.0
***	 Purpose: Simple, yet effective, counting bloom filter implementation
***
***	 License: MIT 2015
***
***	 URL:	https://github.com/barrust/counting_bloom
***
***	 Usage:
***        CountingBloom cb;
***        counting_bloom_init(&cb, 10, 0.01);
***        counting_bloom_add_string(&cb, "google");
***        counting_bloom_add_string(&cb, "facebook");
***        counting_bloom_add_string(&cb, "twitter");
***        counting_bloom_add_string(&cb, "google");
***        if (counting_bloom_check_string(&cb, "google") == COUNTING_BLOOM_SUCCESS) {
***        	printf("'test' is in the counting bloom a maximum of %d times!\n", counting_bloom_get_max_insertions(&cb, "test"));
***        }
***        counting_bloom_remove_string(&cb, "google");
***        counting_bloom_stats(&cb);
***        counting_bloom_destroy(&cb);
***
***	Required Compile Flags: -lm
***
*******************************************************************************/

#ifdef __cplusplus
extern "C" {
#endif


#include <inttypes.h>       /* PRIu64 */

/* https://gcc.gnu.org/onlinedocs/gcc/Alternate-Keywords.html#Alternate-Keywords */
#ifndef __GNUC__
#define __inline__ inline
#endif


#ifdef __APPLE__
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"
#endif

#define COUNTING_BLOOMFILTER_VERSION "1.0.3"
#define COUNTING_BLOOMFILTER_MAJOR 1
#define COUNTING_BLOOMFILTER_MINOR 0
#define COUNTING_BLOOMFILTER_REVISION 3

#define COUNTING_BLOOM_SUCCESS 0
#define COUNTING_BLOOM_FAILURE -1

#define counting_bloom_get_version()	(COUNTING_BLOOMFILTER_VERSION)

typedef uint64_t* (*CountBloomHashFunction)       (int num_hashes, const char* key);

typedef struct counting_bloom_filter {
    /* bloom parameters */
    uint64_t estimated_elements;
    float false_positive_probability;
    unsigned int number_hashes;
    uint64_t number_bits;
    /* bloom filter */
    uint32_t* bloom;
    uint64_t elements_added;
    CountBloomHashFunction hash_function;
    /* on disk handeling */
    short __is_on_disk;
    FILE* filepointer;
    uint64_t __filesize;
} CountingBloom;

/*
    Initialize a standard counting bloom filter in memory; this will provide 'optimal' size
    and hash numbers.

    Estimated elements is 0 < x <= UINT64_MAX.
    False positive rate is 0.0 < x < 1.0
*/
int counting_bloom_init_alt(CountingBloom* cb, uint64_t estimated_elements, float false_positive_rate, CountBloomHashFunction hash_function);
static __inline__ int counting_bloom_init(CountingBloom* cb, uint64_t estimated_elements, float false_positive_rate) {
    return counting_bloom_init_alt(cb, estimated_elements, false_positive_rate, NULL);
}

/* Initialize a counting bloom directly into file; useful if the counting bloom is larger than available RAM */
int counting_bloom_init_on_disk_alt(CountingBloom* cb, uint64_t estimated_elements, float false_positive_rate, const char* filepath, CountBloomHashFunction hash_function);
static __inline__ int counting_bloom_init_on_disk(CountingBloom* cb, uint64_t estimated_elements, float false_positive_rate, const char* filepath) {
    return counting_bloom_init_on_disk_alt(cb, estimated_elements, false_positive_rate, filepath, NULL);
}

/* Print out statistics about the counting bloom filter */
void counting_bloom_stats(const CountingBloom* cb);

/* Release all memory allocated for the counting bloom */
int counting_bloom_destroy(CountingBloom* cb);

/* reset filter to unused state */
int counting_bloom_clear(CountingBloom* cb);

/*  Add a string (or element) to the counting bloom filter */
int counting_bloom_add_string(CountingBloom* cb, const char* key);

/* Add a string to a counting bloom filter using the passed hashes */
int counting_bloom_add_string_alt(CountingBloom* cb, const uint64_t* hashes, unsigned int number_hashes_passed);

/* Check to see if a string (or element) is or is not in the counting bloom */
int counting_bloom_check_string(const CountingBloom* cb, const char* key);

/* Check if a string is in the counting bloom using the passed hashes */
int counting_bloom_check_string_alt(const CountingBloom* cb, const uint64_t* hashes, unsigned int number_hashes_passed);

/* Determine the maximum number of times a string could have been inserted */
int counting_bloom_get_max_insertions(const CountingBloom* cb, const char* key);

/* Determine the maximum number of times an element could have been inserted based on the passed hashes */
int counting_bloom_get_max_insertions_alt(const CountingBloom* cb, const uint64_t* hashes, unsigned int number_hashes_passed);

/* Remove a string from the counting bloom */
int counting_bloom_remove_string(CountingBloom* cb, const char* key);

/* Remove an element from the counting bloom based on the passed hashes */
int counting_bloom_remove_string_alt(CountingBloom* cb, const uint64_t* hashes, unsigned int number_hashes_passed);

/* Export the current counting bloom to file */
int counting_bloom_export(const CountingBloom* cb, const char* filepath);

/* Import a previously exported counting bloom from a file into memory */
int counting_bloom_import_alt(CountingBloom* cb, const char* filepath, CountBloomHashFunction hash_function);
static __inline__ int counting_bloom_import(CountingBloom* cb, const char* filepath) {
    return counting_bloom_import_alt(cb, filepath, NULL);
}

/*
    Import a previously exported counting bloom from a file but do not pull the full bloom into memory.
    This is allows for the speed / storage trade off of not needing to put the full counting bloom
    into RAM.
*/
int counting_bloom_import_on_disk_alt(CountingBloom* cb, const char* filepath, CountBloomHashFunction hash_function);
static __inline__ int counting_bloom_import_on_disk(CountingBloom* cb, const char* filepath) {
    return counting_bloom_import_on_disk_alt(cb, filepath, NULL);
}

/* Calculates the current false positive rate based on the number of inserted elements */
float counting_bloom_current_false_positive_rate(const CountingBloom* cb);

/*
    Generate the number of hashes using the counting blooms hashing function. This is
    useful when needing to either add or check the same string to several counting
    blooms that use the same hash function.

    NOTE: It is up to the caller to free the allocated memory
*/
uint64_t* counting_bloom_calculate_hashes(const CountingBloom* cb, const char* key, unsigned int number_hashes);

/* Count the number of bits set to 1 (i.e., greater than 0) */
uint64_t counting_bloom_count_set_bits(const CountingBloom* cb);

/* Calculate the size the bloom filter will take on disk when exported in bytes */
uint64_t counting_bloom_export_size(const CountingBloom* cb);


#ifdef __cplusplus
} // extern "C"
#endif

#endif /* END COUNTING BLOOM FILTER HEADER */

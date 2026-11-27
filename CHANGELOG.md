# Counting Bloom Filter

### Version 1.1.1 (unreleased)
* Fix: removing a key that was not added (a false positive) whose hashes repeat an index could take that index below zero, wrapping it around to `UINT32_MAX`; `counting_bloom_remove_string_alt()` now fails without changing the filter
* Fix: `elements_added` no longer wraps around when removing with a count of 0
* Fix: `counting_bloom_current_false_positive_rate()` overflowed an `int` once the number of hashes times the elements added passed `INT_MAX`, returning a wrong (even negative) rate
* Fix: a false positive rate above about 0.7 gave 0 hashes, so every check succeeded; `counting_bloom_init()` and `counting_bloom_init_on_disk()` now fail (check the return value before using the filter)
* Fix: a failed hash allocation is now returned as a failure instead of dereferencing `NULL`; `counting_bloom_calculate_hashes()` returns `NULL` if the memory cannot be allocated

### Version 1.1.0
* ***BACKWARD INCOMPATIBLE CHANGES***
    * **NOTE:** Breaks backwards compatibility with previously exported blooms using the default hash!
    * Update to the FNV_1a hash function
    * Simplified hashing at depth by using a seed value

## Version 1.0.3
* Full test suite
* Ensure bloom size to be `uint32_t` instead of `unsigned int`
* Added `counting_bloom_export_size()`
* Added `counting_bloom_clear()`

## Version 1.0.2
* Added CPP Guards and resolved issues for C++ malloc/calloc
* Ensure appropriate usage of `const char*`
* Makefile improvements

### Version 1.0.1
* Changed default hash algorithm to FNV-1a
  * Removed need for -lcrypto
* Unique HashFunction type when using with related libraries

### Version 1.0.0
* Basic counting bloom filter implementation
* Support for on disk reading and writing of the counting bloom
* Passable hashing function if a different hashing function is required
* Ability to add, check, and remove data from the bloom filter
  * Remove is a probabilistic remove and decrements the 'bits'
* Generation of hash array to pass to multiple bloom filters to reduce time
to hash for each filter

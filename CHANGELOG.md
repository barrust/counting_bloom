# Counting Bloom Filter

### FUTURE VERSION
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

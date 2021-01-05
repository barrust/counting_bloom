# Counting Bloom Filter

## Current Version
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

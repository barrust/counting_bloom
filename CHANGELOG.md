## Current Version

### Version 1.0.1
* Changed default hash algorithm to FNV-1a
  * Removed need for -lcrypto

### Version 1.0.0
* Basic counting bloom filter implementation
* Support for on disk reading and writing of the counting bloom
* Passable hashing function if a different hashing function is required
* Ability to add, check, and remove data from the bloom filter
  * Remove is a probabilistic remove and decrements the 'bits'
* Generation of hash array to pass to multiple bloom filters to reduce time
to hash for each filter

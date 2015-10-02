
#include "counting_bloom.h"
#include <openssl/sha.h>

#define verbose 0

int convert_to_upper(char *str) {
	while(*str != '\0') {
		 *str++ = toupper(*str);
	}
}

/*
	Example of generating a custom hashing function; in this case, we want each
	string to be added as uppercase.
*/
uint64_t* sha256_hash(int num_hashes, char *str) {
	// uppercase the string
	char *tmp_str = calloc(strlen(str) + 1, sizeof(char));
	int i = 0;
	for (i = 0; i < strlen(str); i++) {
		tmp_str[i] = toupper(str[i]);
	}
	uint64_t *results = calloc(num_hashes, sizeof(uint64_t));
	unsigned char digest[SHA256_DIGEST_LENGTH];
	for (i = 0; i < num_hashes; i++) {
		SHA256_CTX sha256_ctx;
		SHA256_Init(&sha256_ctx);
		if (i == 0) {
			SHA256_Update(&sha256_ctx, tmp_str, strlen(tmp_str));
		} else {
			SHA256_Update(&sha256_ctx, digest, SHA256_DIGEST_LENGTH);
		}
		SHA256_Final(digest, &sha256_ctx);
		results[i] = (uint64_t) *(uint64_t *)digest % UINT64_MAX;
	}
	free(tmp_str);
	return results;
}

int main(int argc, char **argv) {
	printf("Testing Counting Bloom version %s\n", counting_bloom_get_version());

	CountingBloom cb;
	printf("Build the counting bloom on disk!\n");
	counting_bloom_init_on_disk_alt(&cb, 10, 0.01, "./dist/test.cbm", &sha256_hash);
	counting_bloom_add_string(&cb, "test");
	counting_bloom_add_string(&cb, "out");
	counting_bloom_add_string(&cb, "the");
	counting_bloom_add_string(&cb, "counting");
	counting_bloom_add_string(&cb, "bloom");
	counting_bloom_add_string(&cb, "filter");

	// we can add it multiple times!
	counting_bloom_add_string(&cb, "test");
	counting_bloom_add_string(&cb, "Test"); // in this version, this is the same as all the others!
	counting_bloom_add_string(&cb, "test");
	if (counting_bloom_check_string(&cb, "test") == COUNTING_BLOOM_SUCCESS) {
		printf("'test' was found in the counting bloom with false positive rate of %f!\n", counting_bloom_current_false_positive_rate(&cb));
		printf("'test' is in the counting bloom a maximum of %d times!\n", counting_bloom_get_max_insertions(&cb, "test"));
	} else {
		printf("'test' was not found in the counting bloom!\n");
	}
	printf("Export the Counting Bloom!\n");
	counting_bloom_destroy(&cb);
	printf("Exported and destroyed the original counting bloom!\n\n");

	printf("Re-import the bloom filter on disk!\n");
	CountingBloom cb1;
	counting_bloom_import_on_disk_alt(&cb1, "./dist/test.cbm", &sha256_hash);
	counting_bloom_add_string(&cb1, "outside");
	if (counting_bloom_check_string(&cb1, "test") == COUNTING_BLOOM_SUCCESS) {
		printf("'test' was found in the counting bloom with false positive rate of %f!\n", counting_bloom_current_false_positive_rate(&cb1));
		printf("'test' is in the counting bloom a maximum of %d times!\n", counting_bloom_get_max_insertions(&cb1, "test"));
	} else {
		printf("'test' was not found in the counting bloom!\n");
	}
	if (counting_bloom_check_string(&cb1, "blah") == COUNTING_BLOOM_SUCCESS) {
		printf("'blah' was found in the counting bloom with false positive rate of %f!\n", counting_bloom_current_false_positive_rate(&cb1));
		printf("'blah' is in the counting bloom a maximum of %d times!\n", counting_bloom_get_max_insertions(&cb1, "test"));
	} else {
		printf("'blah' was not found in the counting bloom!\n");
	}
	counting_bloom_destroy(&cb1);
}

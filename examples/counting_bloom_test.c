
#include <stdlib.h>         /* calloc, malloc */
#include <stdio.h>          /* printf */
#include <string.h>         /* strlen */

#include "../src/counting_bloom.h"

#define verbose 0


int main() {
	printf("Testing Counting Bloom version %s\n", counting_bloom_get_version());

	CountingBloom cb;
	counting_bloom_init(&cb, 10, 0.01);
	counting_bloom_add_string(&cb, "test");
	counting_bloom_add_string(&cb, "out");
	counting_bloom_add_string(&cb, "the");
	counting_bloom_add_string(&cb, "counting");
	counting_bloom_add_string(&cb, "bloom");
	counting_bloom_add_string(&cb, "filter");

	// we can add it multiple times!
	counting_bloom_add_string(&cb, "test");
	counting_bloom_add_string(&cb, "Test"); // should not be added to the 'test' strings
	counting_bloom_add_string(&cb, "out");
	counting_bloom_add_string(&cb, "test");
	// print out the numbers if verbose
	if (verbose == 1) {
		for (unsigned int i = 0; i < cb.number_bits; i++) {
			printf("%d\t", cb.bloom[i]);
		}
		printf("\n");
	}


	if (counting_bloom_check_string(&cb, "test") == COUNTING_BLOOM_SUCCESS) {
		printf("'test' was found in the counting bloom with false positive rate of %f!\n", counting_bloom_current_false_positive_rate(&cb));
		printf("'test' is in the counting bloom a maximum of %d times!\n", counting_bloom_get_max_insertions(&cb, "test"));
	} else {
		printf("'test' was not found in the counting bloom!\n");
	}
	counting_bloom_stats(&cb);
	printf("\nRemove 'test' and re-test!!\n");
	counting_bloom_remove_string(&cb, "test");
	if (counting_bloom_check_string(&cb, "test") == COUNTING_BLOOM_SUCCESS) {
		printf("'test' was found in the counting bloom with false positive rate of %f!\n", counting_bloom_current_false_positive_rate(&cb));
		printf("'test' is in the counting bloom a maximum of %d times!\n", counting_bloom_get_max_insertions(&cb, "test"));
	} else {
		printf("'test' was not found in the counting bloom!\n");
	}

	counting_bloom_stats(&cb);
	counting_bloom_destroy(&cb);
}




#include "counting_bloom.h"

static const double LOG_TWO_SQUARED = 0.4804530139182;

/*******************************************************************************
***		PRIVATE FUNCTIONS
*******************************************************************************/
static uint64_t* md5_hash_default(int num_hashes, char *str);
static void calculate_optimal_hashes(CountingBloom *cb);


int counting_bloom_init(CountingBloom *cb, uint64_t estimated_elements, float false_positive_rate, HashFunction hash_function) {
	if(estimated_elements <= 0 || estimated_elements > UINT64_MAX) {
		return COUNTING_BLOOM_FAILURE;
	}
	if (false_positive_rate <= 0.0 || false_positive_rate >= 1.0 ) {
		return COUNTING_BLOOM_FAILURE;
	}
	cb->estimated_elements = estimated_elements;
	cb->false_positive_probability = false_positive_rate;
	calculate_optimal_hashes(cb);
	cb->bloom = calloc(cb->number_bits, sizeof(unsigned int));
	cb->elements_added = 0;
	cb->hash_function = (hash_function == NULL) ? md5_hash_default : hash_function;
	return COUNTING_BLOOM_SUCCESS;
}


int counting_bloom_destroy(CountingBloom *cb) {
	cb->estimated_elements = 0;
	cb->false_positive_probability = 0.0;
	cb->number_hashes = 0;
	cb->number_bits = 0;
	free(cb->bloom);
	cb->bloom = NULL;
	cb->elements_added = 0;
	cb->hash_function = NULL;
	return COUNTING_BLOOM_SUCCESS;
}

int counting_bloom_add_string(CountingBloom *cb, char *str) {
	uint64_t *hashes = counting_bloom_calculate_hashes(cb, str, cb->number_hashes);
	int r = counting_bloom_add_string_alt(cb, hashes, cb->number_hashes);
	free(hashes);
	return r;
}

int counting_bloom_add_string_alt(CountingBloom *cb, uint64_t* hashes, unsigned int number_hashes_passed) {
	if (number_hashes_passed < cb->number_hashes) {
		printf("Error: Not enough hashes were passed!\n");
		return COUNTING_BLOOM_FAILURE;
	}
	int i;
	for (i = 0; i < cb->number_hashes; i++) {
		uint64_t idx = hashes[i] % cb->number_bits;
		if (cb->bloom[idx] < UINT_MAX) {
			cb->bloom[idx]++;
		}
	}
	cb->elements_added++;  // I could be convinced that if it is a duplicate than it shouldn't increment the elements added
	return COUNTING_BLOOM_SUCCESS;
}

int counting_bloom_check_string(CountingBloom *cb, char *str) {
	uint64_t *hashes = counting_bloom_calculate_hashes(cb, str, cb->number_hashes);
	int r = counting_bloom_check_string_alt(cb, hashes, cb->number_hashes);
	free(hashes);
	return r;

}

int counting_bloom_check_string_alt(CountingBloom *cb, uint64_t* hashes, unsigned int number_hashes_passed) {
	if (number_hashes_passed < cb->number_hashes) {
		printf("Error: Not enough hashes were passed!\n");
		return COUNTING_BLOOM_FAILURE;
	}
	int res = COUNTING_BLOOM_SUCCESS;
	int i;
	for (i = 0; i < cb->number_hashes; i++) {
		uint64_t idx = hashes[i] % cb->number_bits;
		if (cb->bloom[idx] == 0) {
			res = COUNTING_BLOOM_FAILURE;
			break;
		}
	}
	return res;
}

// a better way would be to calculate the hashes only once...
int counting_bloom_get_max_insertions(CountingBloom *cb, char *str) {
	uint64_t *hashes = counting_bloom_calculate_hashes(cb, str, cb->number_hashes);
	return counting_bloom_get_max_insertions_alt(cb, hashes, cb->number_hashes);
	free(hashes);
}

int counting_bloom_get_max_insertions_alt(CountingBloom *cb, uint64_t* hashes, unsigned int number_hashes_passed) {
	if (counting_bloom_check_string_alt(cb, hashes, number_hashes_passed) == COUNTING_BLOOM_FAILURE) {
		return 0; // this means it isn't present; fail-quick
	}
	int i, res = UINT_MAX; // set this to the max and work down
	for (i = 0; i < cb->number_hashes; i++) {
		uint64_t idx = hashes[i] % cb->number_bits;
		if (cb->bloom[idx] < res) {
			res = cb->bloom[idx];
		}
	}
	return res;
}

uint64_t* counting_bloom_calculate_hashes(CountingBloom *cb, char *str, unsigned int number_hashes) {
	return cb->hash_function(number_hashes, str);
}

float counting_bloom_current_false_positive_rate(CountingBloom *cb) {
	int num = (cb->number_hashes * -1 * cb->elements_added);
	double d = (num * 1.0) / (cb->number_bits * 1.0);
	double e = exp(d);
	return pow((1 - e), cb->number_hashes);
}










/*******************************************************************************
***		PRIVATE FUNCTIONS
*******************************************************************************/
static void calculate_optimal_hashes(CountingBloom *cb) {
	// calc optimized values
	long n = cb->estimated_elements;
	float p = cb->false_positive_probability;
	uint64_t m = ceil((-n * log(p)) / LOG_TWO_SQUARED);  // AKA pow(log(2), 2);
	unsigned int k = round(log(2.0) * m / n);
	// set paramenters
	cb->number_hashes = k; // should check to make sure it is at least 1...
	cb->number_bits = m;
}

/* NOTE: The caller will free the results */
static uint64_t* md5_hash_default(int num_hashes, char *str) {
	uint64_t *results = calloc(num_hashes, sizeof(uint64_t));
	unsigned char digest[MD5_DIGEST_LENGTH];
	int i;
	for (i = 0; i < num_hashes; i++) {
		MD5_CTX md5_ctx;
		MD5_Init(&(md5_ctx));
		if (i == 0) {
			MD5_Update(&(md5_ctx), str, strlen(str));
		} else {
			MD5_Update(&(md5_ctx), digest, MD5_DIGEST_LENGTH);
		}
		MD5_Final(digest, &(md5_ctx));
		results[i] = (uint64_t) *(uint64_t *)digest % UINT64_MAX;
	}
	return results;
}

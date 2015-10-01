/*******************************************************************************
***
***	 Author: Tyler Barrus
***	 email:  barrust@gmail.com
***
***	 Version:
***
***	 License: MIT 2015
***
*******************************************************************************/
#include "counting_bloom.h"

static const double LOG_TWO_SQUARED = 0.4804530139182;

/*******************************************************************************
***		PRIVATE FUNCTIONS
*******************************************************************************/
static uint64_t* md5_hash_default(int num_hashes, char *str);
static void calculate_optimal_hashes(CountingBloom *cb);
static void write_to_file(CountingBloom *cb, FILE *fp, short on_disk);
static void read_from_file(CountingBloom *cb, FILE *fp, short on_disk, char *filename);


/*******************************************************************************
***		PUBLIC FUNCTION DECLARATIONS
*******************************************************************************/
int counting_bloom_init(CountingBloom *cb, uint64_t estimated_elements, float false_positive_rate) {
	return counting_bloom_init_alt(cb, estimated_elements, false_positive_rate, NULL);
}


int counting_bloom_init_alt(CountingBloom *cb, uint64_t estimated_elements, float false_positive_rate, HashFunction hash_function) {
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
	cb->__is_on_disk = 0;
	cb->hash_function = (hash_function == NULL) ? md5_hash_default : hash_function;
	return COUNTING_BLOOM_SUCCESS;
}


int counting_bloom_destroy(CountingBloom *cb) {
	if (cb->__is_on_disk == 0) {
		free(cb->bloom);
	} else {
		fclose(cb->filepointer);
		munmap(cb->bloom, cb->__filesize);
	}
	cb->estimated_elements = 0;
	cb->false_positive_probability = 0.0;
	cb->number_hashes = 0;
	cb->number_bits = 0;
	cb->bloom = NULL;
	cb->elements_added = 0;
	cb->hash_function = NULL;
	cb->__is_on_disk = 0;
	cb->__filesize = 0;
	cb->filepointer = NULL;
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
	if (cb->__is_on_disk == 1) {
		int offset = sizeof(uint64_t) + sizeof(float);
		fseek(cb->filepointer, offset * -1, SEEK_END);
		fwrite(&cb->elements_added, 1, sizeof(uint64_t), cb->filepointer);
	}
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
	int r = counting_bloom_get_max_insertions_alt(cb, hashes, cb->number_hashes);
	free(hashes);
	return r;
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

int counting_bloom_remove_string(CountingBloom *cb, char *str) {
	uint64_t *hashes = counting_bloom_calculate_hashes(cb, str, cb->number_hashes);
	int r = counting_bloom_remove_string_alt(cb, hashes, cb->number_hashes);
	free(hashes);
	return r;
}

int counting_bloom_remove_string_alt(CountingBloom *cb, uint64_t* hashes, unsigned int number_hashes_passed) {
	if (counting_bloom_check_string_alt(cb, hashes, number_hashes_passed) == COUNTING_BLOOM_FAILURE) {
		return COUNTING_BLOOM_FAILURE; // this means it isn't present; fail-quick
	}
	int i, completly_removed;
	for (i = 0; i < cb->number_hashes; i++) {
		uint64_t idx = hashes[i] % cb->number_bits;
		if (cb->bloom[idx] != UINT_MAX) {
			cb->bloom[idx]--;
		}
	}
	cb->elements_added--; // this would need to be modified if we don't add for each
	return COUNTING_BLOOM_SUCCESS;
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

int counting_bloom_export(CountingBloom *cb, char *filepath) {
	FILE *fp;
	fp = fopen(filepath, "w+b");
	if (fp == NULL) {
		fprintf(stderr, "Can't open file %s!\n", filepath);
		return COUNTING_BLOOM_FAILURE;
	}
	write_to_file(cb, fp, 0);
	fclose(fp);
	return COUNTING_BLOOM_SUCCESS;
}


int counting_bloom_import(CountingBloom *cb, char *filepath) {
	return counting_bloom_import_alt(cb, filepath, NULL);
}

int counting_bloom_import_alt(CountingBloom *cb, char *filepath, HashFunction hash_function) {
	FILE *fp;
	fp = fopen(filepath, "r+b");
	if (fp == NULL) {
		fprintf(stderr, "Can't open file %s!\n", filepath);
		return COUNTING_BLOOM_FAILURE;
	}
	read_from_file(cb, fp, 0, NULL);
	fclose(fp);
	cb->hash_function = (hash_function == NULL) ? md5_hash_default : hash_function;
	return COUNTING_BLOOM_SUCCESS;
}



int counting_bloom_import_on_disk(CountingBloom *cb, char *filepath) {
	return counting_bloom_import_on_disk_alt(cb, filepath, NULL);
}


int counting_bloom_import_on_disk_alt(CountingBloom *cb, char *filepath, HashFunction hash_function) {
	cb->filepointer = fopen(filepath, "r+b");
	if (cb->filepointer == NULL) {
		fprintf(stderr, "Can't open file %s!\n", filepath);
		return COUNTING_BLOOM_FAILURE;
	}
	read_from_file(cb, cb->filepointer, 1, filepath);
	// don't close the file pointer here...
	cb->hash_function = (hash_function == NULL) ? md5_hash_default : hash_function;
	cb->__is_on_disk = 1; // on disk
	return COUNTING_BLOOM_SUCCESS;
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

/* NOTE: this assumes that the file handler is open and ready to use */
static void write_to_file(CountingBloom *cb, FILE *fp, short on_disk) {
	if (on_disk == 0) {
		fwrite(cb->bloom, sizeof(unsigned int), cb->number_bits, fp);
	} else { /*
		// will need to write out everything by hand
		uint64_t i;
		unsigned int q = 0;

		//fwrite(&q, 1, bf->bloom_length, fp);
		for (i = 0; i < cb->number_bits; i++) {
			fwrite(&q, 1, sizeof(unsigned int), fp);
		}
		*/
	}
	fwrite(&cb->estimated_elements, sizeof(uint64_t), 1, fp);
	fwrite(&cb->elements_added, sizeof(uint64_t), 1, fp);
	fwrite(&cb->false_positive_probability, sizeof(float), 1, fp);

}

/* NOTE: this assumes that the file handler is open and ready to use */
static void read_from_file(CountingBloom *cb, FILE *fp, short on_disk, char *filename) {
	int offset = sizeof(uint64_t) * 2 + sizeof(float);
	fseek(fp, offset * -1, SEEK_END);
	fread(&cb->estimated_elements, sizeof(uint64_t), 1, fp);
	fread(&cb->elements_added, sizeof(uint64_t), 1, fp);
	fread(&cb->false_positive_probability, sizeof(float), 1, fp);
	calculate_optimal_hashes(cb);
	rewind(fp);
	if(on_disk == 0) {
		cb->bloom = calloc(cb->number_bits, sizeof(unsigned int));
		fread(cb->bloom, sizeof(unsigned int), cb->number_bits, fp);
	} else {  // this is for on disk implementation which isn't completed yet
		struct stat buf;
		int fd = open(filename, O_RDWR);
		if (fd < 0) {
			perror("open: ");
			exit(1);
		}
		fstat(fd, &buf);
		cb->__filesize = buf.st_size;
		cb->bloom = mmap((caddr_t)0, cb->__filesize, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
		if (cb->bloom == (unsigned int*)-1) {
			perror("mmap: ");
			exit(1);
		}
		// close the file descriptor
		close(fd);
	}
}

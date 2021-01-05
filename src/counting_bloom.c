/*******************************************************************************
***
***	 Author: Tyler Barrus
***	 email:  barrust@gmail.com
***
***	 Version: 1.0.2
***
***	 License: MIT 2015
***
*******************************************************************************/
#include <math.h>           /* pow, exp */
#include <stdlib.h>         /* calloc, malloc */
#include <stdio.h>          /* printf */
#include <string.h>         /* strlen */
#include <limits.h>         /* UINT_MAX */
#include <fcntl.h>          /* open, O_RDWR */
#include <unistd.h>         /* for close */
#include <sys/types.h>      /* */
#include <sys/stat.h>       /* fstat */
#include <sys/mman.h>       /* mmap, mummap */

#include "counting_bloom.h"


static const double LOG_TWO_SQUARED = 0.4804530139182;

/*******************************************************************************
***		PRIVATE FUNCTIONS
*******************************************************************************/
static uint64_t* __default_hash(int num_hashes, const char *str);
static uint64_t __fnv_1a(const char *key);
static void __calculate_optimal_hashes(CountingBloom *cb);
static void __write_to_file(CountingBloom *cb, FILE *fp, short on_disk);
static void __read_from_file(CountingBloom *cb, FILE *fp, short on_disk, const char *filename);
static void __get_additional_stats(CountingBloom *cb, uint64_t *largest, uint64_t *largest_index,uint64_t *els_added, float *fullness);


/*******************************************************************************
***		PUBLIC FUNCTION DECLARATIONS
*******************************************************************************/
int counting_bloom_init(CountingBloom *cb, uint64_t estimated_elements, float false_positive_rate) {
    return counting_bloom_init_alt(cb, estimated_elements, false_positive_rate, NULL);
}


int counting_bloom_init_alt(CountingBloom *cb, uint64_t estimated_elements, float false_positive_rate, CountBloomHashFunction hash_function) {
    if(estimated_elements <= 0 || estimated_elements > UINT64_MAX) {
        return COUNTING_BLOOM_FAILURE;
    }
    if (false_positive_rate <= 0.0 || false_positive_rate >= 1.0 ) {
        return COUNTING_BLOOM_FAILURE;
    }
    cb->estimated_elements = estimated_elements;
    cb->false_positive_probability = false_positive_rate;
    __calculate_optimal_hashes(cb);
    cb->bloom = (unsigned int*)calloc(cb->number_bits, sizeof(unsigned int));
    cb->elements_added = 0;
    cb->__is_on_disk = 0;
    cb->hash_function = (hash_function == NULL) ? __default_hash : hash_function;
    return COUNTING_BLOOM_SUCCESS;
}


int counting_bloom_init_on_disk(CountingBloom *cb, uint64_t estimated_elements, float false_positive_rate, const char *filepath) {
    return counting_bloom_init_on_disk_alt(cb, estimated_elements, false_positive_rate, filepath, NULL);
}


int counting_bloom_init_on_disk_alt(CountingBloom *cb, uint64_t estimated_elements, float false_positive_rate, const char *filepath, CountBloomHashFunction hash_function){
    if(estimated_elements <= 0 || estimated_elements > UINT64_MAX) {
        return COUNTING_BLOOM_FAILURE;
    }
    if (false_positive_rate <= 0.0 || false_positive_rate >= 1.0 ) {
        return COUNTING_BLOOM_FAILURE;
    }
    cb->estimated_elements = estimated_elements;
    cb->false_positive_probability = false_positive_rate;
    __calculate_optimal_hashes(cb);
    cb->elements_added = 0;
    cb->__is_on_disk = 1;
    cb->hash_function = (hash_function == NULL) ? __default_hash : hash_function;

    FILE *fp;
    fp = fopen(filepath, "w+b");
    if (fp == NULL) {
        fprintf(stderr, "Can't open file %s!\n", filepath);
        return COUNTING_BLOOM_FAILURE;
    }
    __write_to_file(cb, fp, 1);
    fclose(fp);
    return counting_bloom_import_on_disk_alt(cb, filepath, hash_function);
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

int counting_bloom_add_string(CountingBloom *cb, const char *str) {
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
    for (unsigned int i = 0; i < cb->number_hashes; ++i) {
        uint64_t idx = hashes[i] % cb->number_bits;
        if (cb->bloom[idx] < UINT_MAX) {
            ++cb->bloom[idx];
        }
    }
    ++cb->elements_added;  // I could be convinced that if it is a duplicate than it shouldn't increment the elements added
    if (cb->__is_on_disk == 1) {
        int offset = sizeof(uint64_t) + sizeof(float);
        fseek(cb->filepointer, offset * -1, SEEK_END);
        fwrite(&cb->elements_added, 1, sizeof(uint64_t), cb->filepointer);
    }
    return COUNTING_BLOOM_SUCCESS;
}

int counting_bloom_check_string(CountingBloom *cb, const char *str) {
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
    for (unsigned int i = 0; i < cb->number_hashes; ++i) {
        uint64_t idx = hashes[i] % cb->number_bits;
        if (cb->bloom[idx] == 0) {
            res = COUNTING_BLOOM_FAILURE;
            break;
        }
    }
    return res;
}

// a better way would be to calculate the hashes only once...
int counting_bloom_get_max_insertions(CountingBloom *cb, const char *str) {
    uint64_t *hashes = counting_bloom_calculate_hashes(cb, str, cb->number_hashes);
    int r = counting_bloom_get_max_insertions_alt(cb, hashes, cb->number_hashes);
    free(hashes);
    return r;
}

int counting_bloom_get_max_insertions_alt(CountingBloom *cb, uint64_t* hashes, unsigned int number_hashes_passed) {
    if (counting_bloom_check_string_alt(cb, hashes, number_hashes_passed) == COUNTING_BLOOM_FAILURE) {
        return 0; // this means it isn't present; fail-quick
    }
    unsigned int res = UINT_MAX; // set this to the max and work down
    for (unsigned int i = 0; i < cb->number_hashes; ++i) {
        uint64_t idx = hashes[i] % cb->number_bits;
        if (cb->bloom[idx] < res) {
            res = cb->bloom[idx];
        }
    }
    return res;
}

int counting_bloom_remove_string(CountingBloom *cb, const char *str) {
    uint64_t *hashes = counting_bloom_calculate_hashes(cb, str, cb->number_hashes);
    int r = counting_bloom_remove_string_alt(cb, hashes, cb->number_hashes);
    free(hashes);
    return r;
}

int counting_bloom_remove_string_alt(CountingBloom *cb, uint64_t* hashes, unsigned int number_hashes_passed) {
    if (counting_bloom_check_string_alt(cb, hashes, number_hashes_passed) == COUNTING_BLOOM_FAILURE) {
        return COUNTING_BLOOM_FAILURE; // this means it isn't present; fail-quick
    }
    for (unsigned int i = 0; i < cb->number_hashes; ++i) {
        uint64_t idx = hashes[i] % cb->number_bits;
        if (cb->bloom[idx] != UINT_MAX) {
            --cb->bloom[idx];
        }
    }
    --cb->elements_added; // this would need to be modified if we don't add for each
    return COUNTING_BLOOM_SUCCESS;
}

uint64_t* counting_bloom_calculate_hashes(CountingBloom *cb, const char *str, unsigned int number_hashes) {
    return cb->hash_function(number_hashes, str);
}

float counting_bloom_current_false_positive_rate(CountingBloom *cb) {
    int num = (cb->number_hashes * -1 * cb->elements_added);
    double d = (num * 1.0) / (cb->number_bits * 1.0);
    double e = exp(d);
    return pow((1 - e), cb->number_hashes);
}

int counting_bloom_export(CountingBloom *cb, const char *filepath) {
    FILE *fp;
    fp = fopen(filepath, "w+b");
    if (fp == NULL) {
        fprintf(stderr, "Can't open file %s!\n", filepath);
        return COUNTING_BLOOM_FAILURE;
    }
    __write_to_file(cb, fp, 0);
    fclose(fp);
    return COUNTING_BLOOM_SUCCESS;
}


int counting_bloom_import(CountingBloom *cb, const char *filepath) {
    return counting_bloom_import_alt(cb, filepath, NULL);
}

int counting_bloom_import_alt(CountingBloom *cb, const char *filepath, CountBloomHashFunction hash_function) {
    FILE *fp;
    fp = fopen(filepath, "r+b");
    if (fp == NULL) {
        fprintf(stderr, "Can't open file %s!\n", filepath);
        return COUNTING_BLOOM_FAILURE;
    }
    __read_from_file(cb, fp, 0, NULL);
    fclose(fp);
    cb->__is_on_disk = 0;  // not on disk
    cb->hash_function = (hash_function == NULL) ? __default_hash : hash_function;
    return COUNTING_BLOOM_SUCCESS;
}



int counting_bloom_import_on_disk(CountingBloom *cb, const char *filepath) {
    return counting_bloom_import_on_disk_alt(cb, filepath, NULL);
}


int counting_bloom_import_on_disk_alt(CountingBloom *cb, const char *filepath, CountBloomHashFunction hash_function) {
    cb->filepointer = fopen(filepath, "r+b");
    if (cb->filepointer == NULL) {
        fprintf(stderr, "Can't open file %s!\n", filepath);
        return COUNTING_BLOOM_FAILURE;
    }
    __read_from_file(cb, cb->filepointer, 1, filepath);
    // don't close the file pointer here...
    cb->hash_function = (hash_function == NULL) ? __default_hash : hash_function;
    cb->__is_on_disk = 1; // on disk
    return COUNTING_BLOOM_SUCCESS;
}


void counting_bloom_stats(CountingBloom *cb) {
    const char* is_on_disk = (cb->__is_on_disk == 0 ? "no" : "yes");
    uint64_t largest, largest_index, calculated_elements;
    float fullness;
    __get_additional_stats(cb, &largest, &largest_index, &calculated_elements, &fullness);
    /* additional stats needed:
    distribution of idx (histogram?)
    which index is used the most?
    */
    printf("CountingBloom\n\
    bits: %" PRIu64 "\n\
    estimated elements: %" PRIu64 "\n\
    number hashes: %d\n\
    max false positive rate: %f\n\
    elements added: %" PRIu64 "\n\
    current false positive rate: %f\n\
    is on disk: %s\n\
    index fullness: %f\n\
    max index usage: %" PRIu64 "\n\
    max index id: %" PRIu64 "\n\
    calculated elements: %" PRIu64 "\n", // use this to make sure the numbers still match
    cb->number_bits, cb->estimated_elements, cb->number_hashes,
    cb->false_positive_probability, cb->elements_added,
    counting_bloom_current_false_positive_rate(cb), is_on_disk,
    fullness, largest, largest_index, calculated_elements);
}


/*******************************************************************************
***		PRIVATE FUNCTIONS
*******************************************************************************/
static void __calculate_optimal_hashes(CountingBloom *cb) {
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
static uint64_t* __default_hash(int num_hashes, const char *str) {
    uint64_t *results = (uint64_t*)calloc(num_hashes, sizeof(uint64_t));
    int i;
    char key[17] = {0};  // largest value is 7FFF,FFFF,FFFF,FFFF
    results[0] = __fnv_1a(str);
    for (i = 1; i < num_hashes; ++i) {
        sprintf(key, "%" PRIx64 "", results[i-1]);
        results[i] = __fnv_1a(key);
    }
    return results;
}

static uint64_t __fnv_1a(const char *key) {
    // FNV-1a hash (http://www.isthe.com/chongo/tech/comp/fnv/)
    int i, len = strlen(key);
    uint64_t h = 14695981039346656073ULL; // FNV_OFFSET 64 bit
    for (i = 0; i < len; ++i){
        h = h ^ (unsigned char) key[i];
        h = h * 1099511628211ULL; // FNV_PRIME 64 bit
    }
    return h;
}

/* NOTE: this assumes that the file handler is open and ready to use */
static void __write_to_file(CountingBloom *cb, FILE *fp, short on_disk) {
    if (on_disk == 0) {
        fwrite(cb->bloom, sizeof(unsigned int), cb->number_bits, fp);
    } else {
        // will need to write out everything by hand
        uint64_t i;
        unsigned int q = 0;
        for (i = 0; i < cb->number_bits; ++i) {
            fwrite(&q, 1, sizeof(unsigned int), fp);
        }
    }
    fwrite(&cb->estimated_elements, sizeof(uint64_t), 1, fp);
    fwrite(&cb->elements_added, sizeof(uint64_t), 1, fp);
    fwrite(&cb->false_positive_probability, sizeof(float), 1, fp);

}

/* NOTE: this assumes that the file handler is open and ready to use */
static void __read_from_file(CountingBloom *cb, FILE *fp, short on_disk, const char *filename) {
    int offset = sizeof(uint64_t) * 2 + sizeof(float);
    fseek(fp, offset * -1, SEEK_END);
    fread(&cb->estimated_elements, sizeof(uint64_t), 1, fp);
    fread(&cb->elements_added, sizeof(uint64_t), 1, fp);
    fread(&cb->false_positive_probability, sizeof(float), 1, fp);
    __calculate_optimal_hashes(cb);
    rewind(fp);
    if(on_disk == 0) {
        cb->bloom = (unsigned int*)calloc(cb->number_bits, sizeof(unsigned int));
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
        cb->bloom = (unsigned int*)mmap((caddr_t)0, cb->__filesize, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
        if (cb->bloom == (unsigned int*)-1) {
            perror("mmap: ");
            exit(1);
        }
        // close the file descriptor
        close(fd);
    }
}


static void __get_additional_stats(CountingBloom *cb, uint64_t *largest, uint64_t *largest_index, uint64_t *els_added, float *fullness) {
    uint64_t i, sum = 0, lar = 0, cnt = 0, lar_idx;
    for (i = 0; i < cb->number_bits; ++i) {
        uint64_t tmp = cb->bloom[i];
        sum += tmp;
        if (tmp > lar) {
            lar = tmp;
            lar_idx = i;
        }
        if (tmp > 0) {
            ++cnt;
        }
    }
    *fullness = (cnt * 1.0) / cb->number_bits;
    *els_added = (sum) / cb->number_hashes;
    *largest = lar;
    *largest_index = lar_idx;
}

all: clean
	gcc counting_bloom.c counting_bloom_test.c -lm -lcrypto -o ./dist/cblm
clean:
	if [ -e ./dist/cblm ]; then rm ./dist/cblm; fi;

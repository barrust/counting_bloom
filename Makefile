all: clean
	gcc counting_bloom.c counting_bloom_test.c -lm -lcrypto -o ./dist/cblm
	gcc counting_bloom.c counting_bloom_test_import_export.c -lm -lcrypto -o ./dist/cblmix
clean:
	if [ -e ./dist/cblm ]; then rm ./dist/cblm; fi;
	if [ -e ./dist/cblmix ]; then rm ./dist/cblmix; fi;
	if [ -e ./dist/test.cbm ]; then rm ./dist/test.cbm; fi;

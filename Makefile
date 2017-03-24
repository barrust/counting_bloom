SRCDIR=src
DISTDIR=dist
CCFLAGS=-lm -Wall -Wpedantic



all: clean countingbloom
	gcc -o $(DISTDIR)/cblm $(DISTDIR)/counting_bloom.o counting_bloom_test.c $(CCFLAGS)
	gcc -o $(DISTDIR)/cblmix $(DISTDIR)/counting_bloom.o counting_bloom_test_import_export.c $(CCFLAGS)
	gcc -o $(DISTDIR)/cblmd $(DISTDIR)/counting_bloom.o counting_bloom_on_disk.c $(CCFLAGS) -lcrypto

countingbloom:
	gcc -c counting_bloom.c -o $(DISTDIR)/counting_bloom.o $(CCFLAGS)

clean:
	if [ -e ./dist/cblm ]; then rm ./dist/cblm; fi;
	if [ -e ./dist/cblmix ]; then rm ./dist/cblmix; fi;
	if [ -e ./dist/cblmd ]; then rm ./dist/cblmd; fi;
	if [ -e ./dist/test.cbm ]; then rm ./dist/test.cbm; fi;

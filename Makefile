SRCDIR=src
DISTDIR=dist
TESTDIR=tests
CCFLAGS=-lm -Wall -Wpedantic

all: countingbloom
	gcc -o $(DISTDIR)/cblm $(DISTDIR)/counting_bloom.o $(TESTDIR)/counting_bloom_test.c $(CCFLAGS)
	gcc -o $(DISTDIR)/cblmix $(DISTDIR)/counting_bloom.o $(TESTDIR)/counting_bloom_test_import_export.c $(CCFLAGS)
	gcc -o $(DISTDIR)/cblmd $(DISTDIR)/counting_bloom.o $(TESTDIR)/counting_bloom_on_disk.c $(CCFLAGS) -lcrypto

countingbloom:
	gcc -c $(SRCDIR)/counting_bloom.c -o $(DISTDIR)/counting_bloom.o $(CCFLAGS)

clean:
	if [ -f "./$(DISTDIR)/counting_bloom.o" ]; then rm -r ./$(DISTDIR)/counting_bloom.o; fi
	if [ -f "./$(DISTDIR)/cblm" ]; then rm -r ./$(DISTDIR)/cblm; fi
	if [ -f "./$(DISTDIR)/cblmix" ]; then rm -r ./$(DISTDIR)/cblmix; fi
	if [ -f "./$(DISTDIR)/cblmd" ]; then rm -r ./$(DISTDIR)/cblmd; fi

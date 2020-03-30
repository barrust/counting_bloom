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
	if [ -d "./$(DISTDIR)/" ]; then rm -rf ./$(DISTDIR)/*; fi

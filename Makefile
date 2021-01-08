CC=gcc
COMPFLAGS=-lm -Wall -Wpedantic -Winline -Wextra -Wno-long-long
DISTDIR=dist
SRCDIR=src
TESTDIR=tests
EXAMPLEDIR=examples
UNKNOWN_PRAGMAS=-Wno-unknown-pragmas

all: countingbloom
	$(CC) -o ./$(DISTDIR)/cblm ./$(DISTDIR)/counting_bloom.o ./$(EXAMPLEDIR)/counting_bloom_test.c $(COMPFLAGS) $(CCFLAGS)
	$(CC) -o ./$(DISTDIR)/cblmix ./$(DISTDIR)/counting_bloom.o ./$(EXAMPLEDIR)/counting_bloom_test_import_export.c $(COMPFLAGS) $(CCFLAGS)
	$(CC) -o ./$(DISTDIR)/cblmd ./$(DISTDIR)/counting_bloom.o ./$(EXAMPLEDIR)/counting_bloom_on_disk.c $(COMPFLAGS) $(CCFLAGS) -lcrypto

debug: COMPFLAGS += -g
debug: all

release: COMPFLAGS += -O3
release: all

test: COMPFLAGS += --coverage
test: countingbloom
	$(CC) ./$(DISTDIR)/counting_bloom.o ./$(TESTDIR)/testsuite.c $(CCFLAGS) $(COMPFLAGS) $(UNKNOWN_PRAGMAS) -o ./$(DISTDIR)/test -g -lcrypto

runtests:
	@ if [ -f "./$(DISTDIR)/test" ]; then ./$(DISTDIR)/test; fi

countingbloom:
	$(CC) -c ./$(SRCDIR)/counting_bloom.c -o ./$(DISTDIR)/counting_bloom.o $(COMPFLAGS) $(CCFLAGS)

clean:
	#library
	if [ -f "./$(DISTDIR)/counting_bloom.o" ]; then rm -r ./$(DISTDIR)/counting_bloom.o; fi
	# examples
	if [ -f "./$(DISTDIR)/cblm" ]; then rm -r ./$(DISTDIR)/cblm; fi
	if [ -f "./$(DISTDIR)/cblmix" ]; then rm -r ./$(DISTDIR)/cblmix; fi
	if [ -f "./$(DISTDIR)/cblmd" ]; then rm -r ./$(DISTDIR)/cblmd; fi
	if [ -f "./$(DISTDIR)/test.cbm" ]; then rm -r ./$(DISTDIR)/test.cbm; fi
	# remove testsuite and coverage items
	if [ -f "./$(DISTDIR)/test" ]; then rm -rf ./$(DISTDIR)/*.gcno; fi
	if [ -f "./$(DISTDIR)/test" ]; then rm -rf ./$(DISTDIR)/*.gcda; fi
	if [ -f "./$(DISTDIR)/test" ]; then rm -r ./$(DISTDIR)/test; fi
	rm -f ./*.gcno
	rm -f ./*.gcda
	rm -f ./*.gcov

all:	testrunner simplecpp

CPPFLAGS ?=
CXXFLAGS = -Wall -Wextra -pedantic -Wcast-qual -Wfloat-equal -Wmissing-declarations -Wmissing-format-attribute -Wpacked -Wredundant-decls -Wundef -Woverloaded-virtual -std=c++11 -g $(CXXOPTS)
LDFLAGS = -g $(LDOPTS)

# Define test source dir macro for compilation (preprocessor flags)
TEST_CPPFLAGS = -DSIMPLECPP_TEST_SOURCE_DIR=\"$(CURDIR)\"

# Only test.o gets the define
test.o: CPPFLAGS += $(TEST_CPPFLAGS)
test.o: CXXFLAGS += -Wno-multichar

%.o: %.cpp	simplecpp.h
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -c $< $(LIB_FUZZING_ENGINE)

fuzz_no.o: fuzz.cpp
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -DNO_FUZZ -c -o $@ $^

testrunner:	test.o	simplecpp.o
	$(CXX) $(LDFLAGS) -o $@ $^

test:	testrunner	simplecpp
	./testrunner
	python3 run-tests.py
	python3 -m pytest integration_test.py -vv

fuzz:	fuzz.o simplecpp.o
	# TODO: use -stdlib=libc++ -lc++
	# make fuzz CXX=clang++ CXXOPTS="-O2 -fno-omit-frame-pointer -g -gline-tables-only -DFUZZING_BUILD_MODE_UNSAFE_FOR_PRODUCTION -fsanitize=address,undefined -fsanitize-address-use-after-scope -fno-sanitize=integer -fno-sanitize-recover=undefined" LIB_FUZZING_ENGINE="-fsanitize=fuzzer"
	$(CXX) $(LDFLAGS) $(CXXFLAGS) -o $@ $^ $(LIB_FUZZING_ENGINE)

no-fuzz:	fuzz_no.o simplecpp.o
	$(CXX) $(LDFLAGS) $(CXXFLAGS) -o $@ $^

selfcheck:	simplecpp
	CXX=$(CXX) ./selfcheck.sh

simplecpp:	main.o simplecpp.o
	$(CXX) $(LDFLAGS) -o $@ $^

clean:
	rm -f testrunner fuzz no-fuzz simplecpp *.o

all:	testrunner simplecpp

CXXFLAGS = -Wall -Wextra -pedantic -Wcast-qual -Wfloat-equal -Wmissing-declarations -Wmissing-format-attribute -Wredundant-decls -Wundef -Wno-multichar -Wold-style-cast -std=c++11 -g $(CXXOPTS)
LDFLAGS = -g $(LDOPTS)

%.o: %.cpp	simplecpp.h
	$(CXX) $(CXXFLAGS) -c $< $(LIB_FUZZING_ENGINE)

fuzz_no.o: fuzz.cpp
	$(CXX) $(CXXFLAGS) -DNO_FUZZ -c -o $@ fuzz.cpp

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
	./selfcheck.sh

simplecpp:	main.o simplecpp.o
	$(CXX) $(LDFLAGS) -o $@ $^

clean:
	rm -f testrunner fuzz no-fuzz simplecpp *.o

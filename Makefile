all:	testrunner simplecpp

CPPFLAGS ?=
CXXFLAGS = -Wall -Wextra -pedantic -Wcast-qual -Wfloat-equal -Wmissing-declarations -Wmissing-format-attribute -Wredundant-decls -Wundef -Wno-multichar -Wold-style-cast -std=c++11 -g $(CXXOPTS)
LDFLAGS = -g $(LDOPTS)

# Define test source dir macro for compilation (preprocessor flags)
TEST_CPPFLAGS = -DSIMPLECPP_TEST_SOURCE_DIR=\"$(CURDIR)\"

# Only test.o gets the define
test.o: CPPFLAGS += $(TEST_CPPFLAGS)

%.o: %.cpp	simplecpp.h
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -c $<

testrunner:	test.o	simplecpp.o
	$(CXX) $(LDFLAGS) simplecpp.o test.o -o testrunner

test:	testrunner	simplecpp
	./testrunner
	python3 run-tests.py
	python3 -m pytest integration_test.py -vv

selfcheck:	simplecpp
	./selfcheck.sh

simplecpp:	main.o simplecpp.o
	$(CXX) $(LDFLAGS) main.o simplecpp.o -o simplecpp

clean:
	rm -f testrunner simplecpp *.o

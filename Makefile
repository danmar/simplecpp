all:	testrunner	simplecpp

CXXFLAGS ?= -Wall -Wextra -pedantic -g -std=c++0x -Wno-long-long

testrunner:	test.cpp	simplecpp.o
	$(CXX) $(CXXFLAGS) simplecpp.o test.cpp -o testrunner

clean:
	rm -f simplecpp.o simplecpp  testrunner

simplecpp.o:	simplecpp.cpp	simplecpp.h
	$(CXX) $(CXXFLAGS) -c simplecpp.cpp

test:	testrunner	simplecpp
	./testrunner && python run-tests.py

simplecpp:	main.cpp	simplecpp.o
	$(CXX) $(CXXFLAGS) main.cpp simplecpp.o -o simplecpp

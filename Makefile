all:	testrunner	simplecpp

testrunner:	test.cpp	simplecpp.o
	$(CXX) -Wall -Wextra -pedantic -g -std=c++0x simplecpp.o test.cpp -o testrunner

simplecpp.o:	simplecpp.cpp	simplecpp.h
	$(CXX) -Wall -Wextra -pedantic -Wno-long-long -g -c simplecpp.cpp

test:	testrunner	simplecpp
	./testrunner && python run-tests.py

simplecpp:	main.cpp	simplecpp.o
	$(CXX) -Wall -g -std=c++0x main.cpp simplecpp.o -o simplecpp


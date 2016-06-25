testrunner:	test.cpp	simplecpp.cpp	simplecpp.h
	g++ -Wall -Wextra -pedantic -g -std=c++11 simplecpp.cpp test.cpp -o testrunner

test:	testrunner
	./testrunner


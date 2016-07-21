testrunner:	test.cpp	simplecpp.o
	g++ -Wall -Wextra -pedantic -g -std=c++11 simplecpp.o test.cpp -o testrunner

simplecpp.o:	simplecpp.cpp	simplecpp.h
	g++ -Wall -Wextra -pedantic -Wno-long-long -g -c simplecpp.cpp

test:	testrunner
	./testrunner


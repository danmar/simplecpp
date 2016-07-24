all:	testrunner	simplecpp

testrunner:	test.cpp	simplecpp.o
	g++ -Wall -Wextra -pedantic -g -std=c++11 simplecpp.o test.cpp -o testrunner

simplecpp.o:	simplecpp.cpp	simplecpp.h
	g++ -Wall -Wextra -pedantic -Wno-long-long -g -c simplecpp.cpp

test:	testrunner
	./testrunner

simplecpp:	main.cpp	simplecpp.o
	g++ -Wall -g -std=c++11 main.cpp simplecpp.o -o simplecpp


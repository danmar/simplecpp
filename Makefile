all:	testrunner	simplecpp

testrunner:	test.o	simplecpp.o
	$(CXX) $(LDFLAGS) simplecpp.o test.o -o testrunner
	
test.o:	test.cpp
	$(CXX) $(CXXFLAGS) -Wall -Wextra -pedantic -g -std=c++0x -c test.cpp

simplecpp.o:	simplecpp.cpp	simplecpp.h
	$(CXX) $(CXXFLAGS) -Wall -Wextra -pedantic -g -std=c++0x -c simplecpp.cpp

test:	testrunner	simplecpp
	g++ -fsyntax-only simplecpp.cpp && ./testrunner && python run-tests.py

simplecpp:	main.cpp	simplecpp.o
	$(CXX) -Wall -g -std=c++0x main.cpp simplecpp.o -o simplecpp

clean:
	rm -f testrunner simplecpp *.o

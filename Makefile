all:	testrunner simplecpp

CXXFLAGS = -Wall -Wextra -pedantic -g -std=c++0x
LDFLAGS = -g

%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $<


testrunner:	test.o	simplecpp.o
	$(CXX) $(LDFLAGS) simplecpp.o test.o -o testrunner
	
test:	testrunner	simplecpp
	g++ -fsyntax-only simplecpp.cpp && ./testrunner && python run-tests.py

simplecpp:	main.o simplecpp.o
	$(CXX) $(LDFLAGS) main.o simplecpp.o -o simplecpp

clean:
	rm -f testrunner simplecpp *.o

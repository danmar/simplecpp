all:	testrunner simplecpp

CXXFLAGS = -Wall -Wextra  -Wabi -Wcast-qual -Wfloat-equal -Wmissing-declarations -Wmissing-format-attribute -Wredundant-decls -Wshadow -pedantic -g -std=c++0x
LDFLAGS = -g

%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $<


testrunner:	test.o	simplecpp.o
	$(CXX) $(LDFLAGS) simplecpp.o test.o -o testrunner
	
test:	testrunner	simplecpp
	# The -std=c++03 makes sure that simplecpp.cpp is C++03 conformant. We don't require a C++11 compiler
	g++ -std=c++03 -fsyntax-only simplecpp.cpp && ./testrunner && python run-tests.py

simplecpp:	main.o simplecpp.o
	$(CXX) $(LDFLAGS) main.o simplecpp.o -o simplecpp

clean:
	rm -f testrunner simplecpp *.o

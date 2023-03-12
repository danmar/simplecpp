all:	testrunner simplecpp

CXXFLAGS = -Wall -Wextra -pedantic -Wcast-qual -Wfloat-equal -Wmissing-declarations -Wmissing-format-attribute -Wredundant-decls -Wundef -Wno-multichar -Wold-style-cast -std=c++0x -g
LDFLAGS = -g

%.o: %.cpp	simplecpp.h
	$(CXX) $(CXXFLAGS) -c $<


testrunner:	test.o	simplecpp.o
	$(CXX) $(LDFLAGS) simplecpp.o test.o -o testrunner

test:	testrunner	simplecpp
	# The -std=c++03 makes sure that simplecpp.cpp is C++03 conformant. We don't require a C++11 compiler
	g++ -std=c++03 -fsyntax-only simplecpp.cpp
	./testrunner
	python3 run-tests.py

selfcheck:	simplecpp
	./selfcheck.sh

simplecpp:	main.o simplecpp.o
	$(CXX) $(LDFLAGS) main.o simplecpp.o -o simplecpp

clean:
	rm -f testrunner simplecpp *.o

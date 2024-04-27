.DELETE_ON_ERROR:

src = $(wildcard *.cpp)
obj = $(filter-out main.o test.o,$(src:.cpp=.o))

includes = -I../cbraid/include -L../cbraid/lib
CFLAGS = -Wall -Werror -Wpedantic -Wconversion -lcbraid -std=c++20 -O3


all: bin/test bin/main

bin/test: $(obj) test.o
	$(CXX) $(includes) -o $@ $^ $(CFLAGS)

bin/main: $(obj) main.o
	$(CXX) $(includes) -o $@ $^ $(CFLAGS)

%.o : %.cpp
	$(CXX) $(includes) -c -o $@ $^ $(CFLAGS)

clean:
	rm -f $(obj) main.o test.o bin/*


.PHONY: clean all

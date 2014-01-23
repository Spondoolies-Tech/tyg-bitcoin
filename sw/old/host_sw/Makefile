CC      = g++
CFLAGS  = 
LDFLAGS = -lpthread

all: spond_test spond_lib.a spond_test_fake spond_lib_fake.a

spond_test: spond_test.cpp spond_lib.h  spond_lib.cpp
	g++ -g spond_lib.cpp          spond_test.cpp -o spond_test $(CFLAGS) $(LDFLAGS) 


spond_test_fake: spond_test.cpp spond_lib.h  spond_lib_fake.cpp
	g++ -g spond_lib_fake.cpp    spond_test.cpp -o spond_test_fake $(CFLAGS) $(LDFLAGS) 
	

spond_lib.o: spond_lib.h spond_lib.cpp
	g++ -c spond_lib.cpp $(CFLAGS) $(LDFLAGS) 


spond_lib.a : spond_lib.o 
	ar cr  spond_lib.a spond_lib.o 



spond_lib_fake.o: spond_lib.h spond_lib_fake.cpp
	g++ -c spond_lib_fake.cpp $(CFLAGS) $(LDFLAGS) 


spond_lib_fake.a : spond_lib_fake.o 
	ar cr  spond_lib_fake.a spond_lib_fake.o 









.PHONY: clean cleanest

clean:
	rm spond_test  spond_test_fake

cleanest: clean
	rm spond_test spond_test_fake


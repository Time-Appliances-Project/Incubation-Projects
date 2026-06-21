CC = g++
CFLAGS = -O2 -Wall -std=c++17 -pthread
LDFLAGS = -levent -levent_core -levent_pthreads -lpthread -lrt

all: ptptool

ptptool: ptptool.o
	$(CC) -o $@ $^ $(LDFLAGS)

%.o: %.cpp
	$(CC) $(CFLAGS) -o $@ -c $<

.PHONY: clean format

clean:
	-rm -f *.o *.log ptptool

format:
	clang-format -i *.cpp

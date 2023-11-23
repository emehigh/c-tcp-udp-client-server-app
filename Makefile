# Exemplu de Makefile pentru soluții scrise în C++.

CC = g++
CCFLAGS = -Wall -Wextra -std=c++17 -O0 -lm

.PHONY: build clean

build: subscriber server

# Nu uitați să modificați numele surselor și, eventual, ale executabilelor.
subscriber: subscriber.cpp
	$(CC) -o $@ $^ $(CCFLAGS)
server: server.cpp
	$(CC) -o $@ $^ $(CCFLAGS)

# Vom șterge executabilele.
clean:
	rm -f server subscriber
#p2 p3 p4

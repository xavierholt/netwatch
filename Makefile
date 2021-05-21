OBJECTS  = Address.o DNSProxy.o Server.o Watcher.o
BINARIES = main.out prox.out

CXX_FLAGS = -std=c++20 -Wall -Wextra -Werror -g

all: $(BINARIES)

main.out: main.o $(OBJECTS) | Address.h DNSProxy.h Server.h Watcher.h
	${CXX} $(CXX_FLAGS) -o "$@" $+ -lpcap

prox.out: prox.o Address.o DNSProxy.o | Address.h DNSProxy.h
	${CXX} $(CXX_FLAGS) -o "$@" $+

%.o: %.cpp
	${CXX} $(CXX_FLAGS) -c -o "$@" "$<"

Watcher.cpp: Watcher.h Address.h  DNSProxy.h Headers.h
	touch "$@"



# Protocoale de comunicatii:
# Laborator 8: Multiplexare
# Makefile

CFLAGS = -Wall -g

# Portul pe care asculta serverul (de completat)
PORT = 12345

# Adresa IP a serverului (de completat)
IP_SERVER = 127.0.0.1

all: server subscriber

#ruleaza testul
run_test:
	sudo python3 test.py

# Compileaza server.c
server: server.c

# Compileaza client.c
subscriber: subscriber.c -lm

.PHONY: clean run_server run_subscriber

# Ruleaza serverul
run_server:
	./server ${PORT}

# Ruleaza clientul
run_subscriber:
	./subscriber ${IP_SERVER} ${PORT}

clean:
	rm -f server subscriber

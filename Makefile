# ── QuizArena Makefile ────────────────────────────────────────────────────────
CC     = gcc
CFLAGS = -Wall -pthread
BRIDGE = bridge.js

.PHONY: all install run-server run-bridge stop clean help

help:
	@echo ""
	@echo "  QuizArena — build & run"
	@echo ""
	@echo "  make all          Build server + client (C)"
	@echo "  make install      Install Node.js deps for bridge"
	@echo "  make run-server   Start the C server  (port 5000)"
	@echo "  make run-bridge   Start the web bridge (port 8080)"
	@echo "  make stop         Kill processes on ports 5000 and 8080"
	@echo "  make clean        Remove binaries + node_modules"
	@echo ""

all: server client

server: server.c
	$(CC) $(CFLAGS) -o server server.c

client: client.c
	$(CC) $(CFLAGS) -o client client.c

install:
	npm init -y
	npm install ws

run-server: server
	./server

run-bridge:
	node $(BRIDGE)

stop:
	@fuser -k 5000/tcp 2>/dev/null || true
	@fuser -k 4444/tcp 2>/dev/null || true
	@echo "Stopped."

clean:
	rm -f server client
	rm -rf node_modules package.json package-lock.json
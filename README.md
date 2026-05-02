# Project Idea
A server hosts a quiz game.

-Server waits for 3 clients
-When all connect, game starts
-Server sends questions to all players
-Clients answer
-Server checks answers
-Scores updated safely
-Final winner displayed
 
## Architecture
           QUIZ SERVER (Your PC)
        -------------------------
        waits on port 5000
        accepts 3 clients
        creates thread per client

       /          |          \
 Client1      Client2      Client3

## Files Logic
-server.c:

Main server
accepts clients
threads
game logic

-client.c:

Player program
connects to server
receives questions
sends answers


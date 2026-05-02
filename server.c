#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <string.h>

#define PORT 5000
#define MAX_PLAYERS 3
#define NUM_QUESTIONS 3


int client_sockets[MAX_PLAYERS];
int scores[MAX_PLAYERS] = {0};

int connected = 0;
int answers_received = 0;
char answers[MAX_PLAYERS];

pthread_mutex_t mutex;
pthread_cond_t cond_all_connected;
pthread_cond_t cond_all_answered;
pthread_cond_t cond_next_question;   

int current_question = 0;            

// ---------------- QUESTIONS ----------------
char *questions[NUM_QUESTIONS] = {
    "Q1: Capital of France?\nA) Rome\nB) Paris\nC) Madrid\nD) Berlin\nAnswer: ",
    "Q2: 2 + 2 = ?\nA) 3\nB) 4\nC) 5\nD) 6\nAnswer: ",
    "Q3: TCP is?\nA) Protocol\nB) Language\nC) OS\nD) Game\nAnswer: "
};

char correct[NUM_QUESTIONS] = {'B', 'B', 'A'};


void send_msg(int sock, const char *msg) {
    char buffer[1200];
    int len = strlen(msg);

    sprintf(buffer, "%d\n", len);
    send(sock, buffer, strlen(buffer), 0);
    send(sock, msg, len, 0);
}


void* client_handler(void* arg) {
    int id = *(int*)arg;
    free(arg);

    
    pthread_mutex_lock(&mutex);
    connected++;
    if (connected == MAX_PLAYERS)
        pthread_cond_broadcast(&cond_all_connected);
    else
        pthread_cond_wait(&cond_all_connected, &mutex);
    pthread_mutex_unlock(&mutex);

    
    for (int q = 0; q < NUM_QUESTIONS; q++) {
        char buffer[10] = {0};

        int r = recv(client_sockets[id], buffer, sizeof(buffer) - 1, 0);
        if (r <= 0) break;

        pthread_mutex_lock(&mutex);

        answers[id] = buffer[0];
        answers_received++;

        if (answers_received == MAX_PLAYERS)
            pthread_cond_signal(&cond_all_answered);

        
        
        if (q < NUM_QUESTIONS - 1) {
            int my_q = q;
            while (current_question == my_q)
                pthread_cond_wait(&cond_next_question, &mutex);
        }

        pthread_mutex_unlock(&mutex);
    }

    return NULL;
}


int main() {
    int server_fd;
    pthread_t threads[MAX_PLAYERS];

    struct sockaddr_in server_addr, client_addr;
    socklen_t client_len = sizeof(client_addr);

    pthread_mutex_init(&mutex, NULL);
    pthread_cond_init(&cond_all_connected, NULL);
    pthread_cond_init(&cond_all_answered, NULL);
    pthread_cond_init(&cond_next_question, NULL);   

    server_fd = socket(AF_INET, SOCK_STREAM, 0);

    
    int opt = 1;
    setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(PORT);

    bind(server_fd, (struct sockaddr*)&server_addr, sizeof(server_addr));
    listen(server_fd, MAX_PLAYERS);

    printf("Server running...\nWaiting for %d players...\n", MAX_PLAYERS);

    
    for (int i = 0; i < MAX_PLAYERS; i++) {
        client_sockets[i] = accept(server_fd,
                                    (struct sockaddr*)&client_addr,
                                    &client_len);

        int *id = malloc(sizeof(int));
        *id = i;

        pthread_create(&threads[i], NULL, client_handler, id);

        printf("Player %d connected\n", i + 1);
    }

    
    for (int q = 0; q < NUM_QUESTIONS; q++) {

        
        pthread_mutex_lock(&mutex);
        answers_received = 0;
        pthread_mutex_unlock(&mutex);

        
        for (int i = 0; i < MAX_PLAYERS; i++) {
            send_msg(client_sockets[i], questions[q]);
        }

        
        pthread_mutex_lock(&mutex);
        while (answers_received < MAX_PLAYERS)
            pthread_cond_wait(&cond_all_answered, &mutex);
        pthread_mutex_unlock(&mutex);

        
        char result_msg[256];
        int offset = 0;
        offset += snprintf(result_msg + offset, sizeof(result_msg) - offset,
                           "Correct answer: %c\nScores so far:\n", correct[q]);

        for (int i = 0; i < MAX_PLAYERS; i++) {
            if (answers[i] == correct[q])
                scores[i]++;
            offset += snprintf(result_msg + offset, sizeof(result_msg) - offset,
                               "  Player %d: %d\n", i + 1, scores[i]);
        }

       
        for (int i = 0; i < MAX_PLAYERS; i++) {
            send_msg(client_sockets[i], result_msg);
        }

        
        pthread_mutex_lock(&mutex);
        current_question++;
        pthread_cond_broadcast(&cond_next_question);
        pthread_mutex_unlock(&mutex);
    }

    
    int winner = 0;
    char final_msg[256];
    int offset = 0;

    offset += snprintf(final_msg + offset, sizeof(final_msg) - offset,
                       "\n--- FINAL SCORES ---\n");

    for (int i = 0; i < MAX_PLAYERS; i++) {
        offset += snprintf(final_msg + offset, sizeof(final_msg) - offset,
                           "Player %d: %d\n", i + 1, scores[i]);
        if (scores[i] > scores[winner])
            winner = i;
    }
    snprintf(final_msg + offset, sizeof(final_msg) - offset,
             "WINNER: Player %d!\n", winner + 1);


    for (int i = 0; i < MAX_PLAYERS; i++) {
        send_msg(client_sockets[i], final_msg);
        close(client_sockets[i]);
    }

    printf("%s", final_msg);
    for (int i = 0; i < MAX_PLAYERS; i++)
        pthread_join(threads[i], NULL);

    pthread_mutex_destroy(&mutex);
    pthread_cond_destroy(&cond_all_connected);
    pthread_cond_destroy(&cond_all_answered);
    pthread_cond_destroy(&cond_next_question);

    close(server_fd);
    return 0;
}
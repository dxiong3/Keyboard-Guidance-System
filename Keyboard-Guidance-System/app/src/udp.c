#include <sys/socket.h>
#include <stdio.h> 
#include <string.h>
#include <unistd.h> 
#include <netinet/in.h>
#include <pthread.h>
#include <stdbool.h>
#include <malloc.h>
#include "udp.h"
#include "shutdown.h"
#include "midiController.h"

// References: Slide deck 06 LinuxProgramming from Dr.Brian
// https://www.cs.princeton.edu/courses/archive/spring14/cos461/docs/rec01-sockets.pdf
// https://vinodthebest.wordpress.com/category/c-programming/c-network-programming/

#define PORT 12345 
#define MAX_LEN 4096
#define PACKET_SIZE 1500 // bytes
#define NUM_COMMAND_SUPPORT 8 // update while implementing
#define TEMP_PREV_COMMAND "temp prev command\n"

struct ReponseMessage {
    int numPackets;
    char* packetContent[PACKET_SIZE];
};

typedef struct ReponseMessage (*CommandFunction)();

struct CommandToFunction {
    char* command;
    CommandFunction function;
    enum Command command_enum;
};

pthread_t udp_id;
pthread_mutex_t udp_mutex;
static enum Command prevCommand; 
static bool isValidCommand;
static bool shutDown;

// Thread function
void *udpServer () {

    int socket_fd;
    struct sockaddr_in servaddr, client; 

    memset(&servaddr, 0, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
    servaddr.sin_port = htons(PORT); 

    // Create and bind to socket
    socket_fd = socket(PF_INET, SOCK_DGRAM, 0); 
    if (socket_fd < 0) {
        printf("Socket Error.\n");
    }
    if (bind(socket_fd, (struct sockaddr*) &servaddr, sizeof(servaddr)) < 0){
        printf("Error on binding.\n");
    }

    // keep the server running after processing one client request
    while (shutDown == false) {
        unsigned int sin_len = sizeof(client);
        char messageRx[MAX_LEN];

        int bytesRx = recvfrom(socket_fd, messageRx, MAX_LEN-1, 0, (struct sockaddr *)&client, &sin_len);
        // Uncomment to test client address info
        // if (inet_ntop(AF_INET, &(client.sin_addr), clientAddress, INET_ADDRSTRLEN) != NULL) {
            // printf("Client info %s, %d\n", clientAddress, ntohs(client.sin_port));
        // }        
        if (bytesRx < 0) {
            printf("Receive error.\n");
        } 
        messageRx[bytesRx] = 0; // Null terminated string
        // printf("Message received (%d) bytes: %s\n", bytesRx, messageRx);

        // Create the reply message and send
        char messageTx[MAX_LEN];
        struct ReponseMessage responseMsg = UDP_processClientCommand(messageRx);
        for (int i = 0; i < responseMsg.numPackets; i++) {
            // printf("Curr packet content is %s\n", (responseMsg.packetContent)[i]); // testing curr packet content 
            snprintf(messageTx, MAX_LEN, "%s\n", (responseMsg.packetContent)[i]);
            sendto(socket_fd, messageTx, strlen(messageTx), 0, (struct sockaddr *)&client, sin_len);
            messageTx[0] = '\0';
        }
    }
    close(socket_fd);
    return NULL;
}

struct ReponseMessage UDP_processClientCommand(char* messageRx) {

    // Create the command to function map
    struct CommandToFunction map[] = {
        {"help",  UDP_commandHelp,       HELP},
        {"?",     UDP_commandHelp,       HELP},
        {"twinkle", UDP_commandTwinkle, UDP_TWINKLE},
        {"bach", UDP_commandBach, UDP_BACH},
        {"zelda", UDP_commandZelda, UDP_ZELDA},
        {"pokemon", UDP_commandPokemon, UDP_POKEMON},
        {"birthday", UDP_commandBirthday, UDP_BIRTHDAY},
        {"popsong", UDP_commandPopSong, UDP_POPSONG},
    };

    char clientCommand[MAX_LEN];
    char* token;
    CommandFunction targetFunction = NULL;
    struct ReponseMessage responseMessage;
    isValidCommand = false;

    if (strncmp(messageRx, "stop", 4) == 0) {
        // printf("Command is stop.\n");
        UDP_commandStop();
        return responseMessage;
    } 

    clientCommand[strcspn(clientCommand, "\n")] = 0; // remove the line feed

    if (((strlen(messageRx) == 1) && (messageRx[0] == '\n'))) {
        // printf("Current command is <enter>\n");
        responseMessage = UDP_commandBlank();
    } else {
        token = strtok(messageRx, " \n\0"); // Tokenize by either space or \n
        if (token != NULL) {
            snprintf(clientCommand, MAX_LEN - 1, "%s\n", token);
        } else {
            printf("Tokenize error.\n");
        }

        for (int i = 0; i < NUM_COMMAND_SUPPORT; i++) {
            clientCommand[strcspn(clientCommand, "\n")] = 0; // remove the line feed

            if (strcmp(clientCommand, map[i].command) == 0) {
                isValidCommand = true;

                pthread_mutex_lock(&udp_mutex);
                targetFunction = map[i].function;
                responseMessage = targetFunction();
                pthread_mutex_unlock(&udp_mutex);

                prevCommand = map[i].command_enum;
                // printf("----------\n");
                break;
            }
        }
        if (!isValidCommand) {
            printf("oops bad command\n");
            return responseMessage;
        }
    }
    return responseMessage;
}

struct ReponseMessage UDP_commandHelp(void) {
    char *helpReply = "Accepted command examples:\ntwinkle          -- plays Twinkle, Twinkle\nbach             -- plays Bach Minuet in G\nzelda            -- plays Zelda's theme song\npokemon          -- plays Pokemon's theme song\nbirthday         -- plays Happy Birthday\npopsong          -- plays a special pop song\nstop             -- cause the server program to end.\n";
    struct ReponseMessage responseMessage;
    responseMessage.numPackets = 1;
    (responseMessage.packetContent)[0] = helpReply;
    return responseMessage;
}

struct ReponseMessage UDP_commandTwinkle(void) {
    char* countReply = malloc(MAX_LEN);
    MidiController_setSong(TWINKLE);
    int song = MidiController_getCurrentSong();
    snprintf(countReply, MAX_LEN, "%d", song);
    struct ReponseMessage responseMessage;
    responseMessage.numPackets = 1;
    (responseMessage.packetContent)[0] = countReply;
    return responseMessage;
}

struct ReponseMessage UDP_commandBach(void) {
    char* countReply = malloc(MAX_LEN);
    MidiController_setSong(BACH);
    int song = MidiController_getCurrentSong();
    snprintf(countReply, MAX_LEN, "%d", song);
    struct ReponseMessage responseMessage;
    responseMessage.numPackets = 1;
    (responseMessage.packetContent)[0] = countReply;
    return responseMessage;
}

struct ReponseMessage UDP_commandZelda(void) {
    char* countReply = malloc(MAX_LEN);
    MidiController_setSong(ZELDA);
    int song = MidiController_getCurrentSong();
    snprintf(countReply, MAX_LEN, "%d", song);
    struct ReponseMessage responseMessage;
    responseMessage.numPackets = 1;
    (responseMessage.packetContent)[0] = countReply;
    return responseMessage;
}
struct ReponseMessage UDP_commandPokemon(void) {
    char* countReply = malloc(MAX_LEN);
    MidiController_setSong(POKEMON);
    int song = MidiController_getCurrentSong();
    snprintf(countReply, MAX_LEN, "%d", song);
    struct ReponseMessage responseMessage;
    responseMessage.numPackets = 1;
    (responseMessage.packetContent)[0] = countReply;
    return responseMessage;
}
struct ReponseMessage UDP_commandBirthday(void) {
    char* countReply = malloc(MAX_LEN);
    MidiController_setSong(BIRTHDAY);
    int song = MidiController_getCurrentSong();
    snprintf(countReply, MAX_LEN, "%d", song);
    struct ReponseMessage responseMessage;
    responseMessage.numPackets = 1;
    (responseMessage.packetContent)[0] = countReply;
    return responseMessage;
}
struct ReponseMessage UDP_commandPopSong(void) {
    char* countReply = malloc(MAX_LEN);
    MidiController_setSong(POPSONG);
    int song = MidiController_getCurrentSong();
    snprintf(countReply, MAX_LEN, "%d", song);
    struct ReponseMessage responseMessage;
    responseMessage.numPackets = 1;
    (responseMessage.packetContent)[0] = countReply;
    return responseMessage;
}


struct ReponseMessage UDP_commandBlank(void) {
    struct CommandToFunction map[] = {
        {"help",  UDP_commandHelp,       HELP},
        {"?",     UDP_commandHelp,       HELP},
        {"twinkle", UDP_commandTwinkle, UDP_TWINKLE},
        {"bach", UDP_commandBach, UDP_BACH},
        {"zelda", UDP_commandZelda, UDP_ZELDA},
        {"pokemon", UDP_commandPokemon, UDP_POKEMON},
        {"birthday", UDP_commandBirthday, UDP_BIRTHDAY},
        {"popsong", UDP_commandPopSong, UDP_POPSONG},
    };
    CommandFunction targetFunction = NULL;
    targetFunction = map[prevCommand].function;
    struct ReponseMessage responseMessage = targetFunction();

    return responseMessage;
}

void UDP_commandStop(void){
    isValidCommand = false;
    shutDown = true;
    Shutdown_triggerShutdown();
    printf("Program terminating.\n");
}

void UDP_init(void) {
    printf("UDP_init...\n");
    shutDown = false;
    pthread_create(&udp_id, NULL, udpServer, NULL); 
}

void UDP_cleanup(void) {
    printf("UDP_cleanup...\n");
    pthread_join(udp_id, NULL);
}


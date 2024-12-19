// This module listen to port 12345 for incoming UDP packets (use a thread). 
// It treats each packet as a command to respond to: 
// reply back to the sender with one or more UDP packets containing the “return” message (plain text).
// currently has issues when interacting with MIDI events 

#ifndef _UDP_H_
#define _UDP_H_

// enum of all the different commands
enum Command {
    HELP = 0, 
    UDP_TWINKLE,
    UDP_BACH,
    UDP_ZELDA,
    UDP_POKEMON,
    UDP_BIRTHDAY,
    UDP_POPSONG,
    MAX_NUMBER_OF_COMMANDS,
    STOP,
    BLANK
};

void UDP_init(void);
void UDP_cleanup(void);

// This function takes in a command from the client input
// and process the command by calling the corresponding functions.
struct ReponseMessage UDP_processClientCommand(char* command);

// The below functions are implemented to generate the ResponseMessage to send back to the client.
// They will be called inside the UDP_processClientCommand() function.
struct ReponseMessage UDP_commandHelp(void);
void UDP_commandStop(void);
struct ReponseMessage UDP_commandBlank(void);

struct ReponseMessage UDP_commandTwinkle(void);
struct ReponseMessage UDP_commandBach(void);
struct ReponseMessage UDP_commandZelda(void);
struct ReponseMessage UDP_commandPokemon(void);
struct ReponseMessage UDP_commandBirthday(void);
struct ReponseMessage UDP_commandPopSong(void);

#endif

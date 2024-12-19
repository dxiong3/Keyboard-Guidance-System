// Continue sampling the midi controller to get the notes being played

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <unistd.h>
#include <pthread.h>
#include <stdbool.h>

#include "hal/midiReader.h"
#include "midiController.h"
#include "shutdown.h"
#include "timeDelay.h"

#define BUFFER_SIZE 4096
#define MAX_NUM_OF_NOTES 13

// the current note that needs to be played with queue sound
static enum note noteToPlay;
// flag to let queue sound know to play new note
static bool needToPlayNote;

static FILE *file = NULL;
static pthread_t midiThread;
static pthread_mutex_t midiReaderMutex;

// variables for chord logic
struct chord currentChord;
static bool stopping = false;
static bool isNotePlayed = false;
// list to store current chord
enum note list[MAX_NUM_OF_NOTES] = {0};

// struct for info on each note
struct note_info {
    int note_value;
    enum note note;
    char* note_name;
};

// notes struct to connect enum notes to string output
static struct note_info notes[] = {
    {60, C,       "C"},
    {61, C_SHARP, "C# / Db"},
    {62, D,       "D"},
    {63, D_SHARP, "D# / Eb"},
    {64, E,       "E"},
    {65, F,       "F"},
    {66, F_SHARP, "F# / Gb"},
    {67, G,       "G"},
    {68, G_SHARP, "G# / Ab"},
    {69, A,       "A"},
    {70, A_SHARP, "A# / Bb"},
    {71, B,       "B"},
    {72, C8,      "C8"},
};

// gets the midi key when played
static char *executeAmidi(FILE *pipe);
// get the midi output
static int parseMIDIOutput(char *midiOutput);

static void* midiReaderThread() {
    char* midiOutput;
    while(!Shutdown_isShutdown()) {

        // Execute amidi and get its output
        // eg outputs 90 3C 33, then 80 3C 0 when released
        midiOutput = executeAmidi(file);

        // printf("raw midi output from executeAimidi: %s\n", midiOutput);

        // Parse MIDI output and extract note value
        int played_key = parseMIDIOutput(midiOutput);

        // if a note is played and it's new (ie flag is set to false)
        // flag should be set to false when it's 80
        if (played_key != -1 && !needToPlayNote) {
            // printf("Note Played: %s, %d\n", MidiReader_noteToString(played_key), MidiReader_intToNote(played_key));

            // reads the note played and sets the note to play
            // allows for SRC functions to get the note and play it with ALSA
            if (MidiReader_noteToString(played_key) != "Unknown"){
                enum note note = MidiReader_intToNote(played_key);
                MidiReader_setNoteToPlay(note);
                MidiReader_setNeedToPlayNote(true);
            }
        }

        // wait for a moment before checking for MIDI events again
        // prevents multiple input from same note played
        sleepForMs(10);
    }

    // free allocated memory
    free(midiOutput);

    return NULL;
}

// init function
void MidiReader_init(void) {
    printf("Press a button on the MIDI controller...\n");

    // command to be run for continuous input from MIDI controller
    const char *command = "amidi -d -p hw:1,0,0";
    file = popen(command, "r");
    if (!file) {
        perror("popen");
        exit(1);
    }

    needToPlayNote = false;

    pthread_mutex_init(&midiReaderMutex, NULL);

    pthread_create(&midiThread, NULL, midiReaderThread, NULL);
}

void MidiReader_cleanup(void) {
    printf("IN midi reader cleanup \n");
    pclose(file);
    pthread_join(midiThread, NULL);
    pthread_mutex_destroy(&midiReaderMutex);
    printf("midi reader cleanup done\n");
}

// function to parse the midi output into int representation
// consulted chatGPT for tokenizing output
static int parseMIDIOutput(char *midiOutput) {
    // midiOutput should look like this initially 90 3E 31

    char *token;
    int note = -1; // Default value if note is not found

    // tokenize MIDI output
    // should get the first part, which is 90
    token = strtok(midiOutput, " ");

    pthread_mutex_lock(&midiReaderMutex);
    while(token != NULL) {

        // 90 represents note being played
        if (strcmp(token, "90") == 0) {

            // get the next part of the raw midi output
            // should be the note in hex eg 3E 
            token = strtok(NULL, " ");
            if (token != NULL) {
                // convert hexadecimal note value to decimal
                note = (int) strtol(token, NULL, 16);
                // printf("parsedNote: %d\n", note);

                if(!isNotePlayed) {
                    isNotePlayed = true;

                    // get the resulting note and reset the list
                    enum note result = MidiReader_intToNote(note);
                    memset(list, -1, MAX_NUM_OF_NOTES);

                    // store the notes played in a list
                    list[0] = result;
                    struct chord chord = {list, 1, false};
                    currentChord = chord;
                    // printf("Current Note: %d\n", currentChord.currentNotes[0]);
                } 
                else {
                    enum note result = MidiReader_intToNote(note);
                    int num = currentChord.numNotes;
                    currentChord.currentNotes[num] = result;
                    // printf("Current Note: %d\n", currentChord.currentNotes[num]);
                    currentChord.numNotes++;
                }
                break;
            }
            isNotePlayed = true;
        } 
        // 80 represents note being released
        else if (strcmp(token, "80") == 0) {
            isNotePlayed = false;

            // set the chord
            MidiController_setChord(currentChord);
            note = -1;

            break;
        }
        token = strtok(NULL, " ");
    }
    pthread_mutex_unlock(&midiReaderMutex);
    return note;
}

// function to execute amidi as a pipe to continuously sample the midi output
// consulted chatGPT on set and select functionalities
static char *executeAmidi(FILE *pipe) {
    // initialize output variable with empty string and set it to empty
    char *output = malloc(BUFFER_SIZE);
    if (!output) {
        perror("malloc");
        exit(1);
    }
    output[0] = '\0'; 

    // buffer string to keep track of current input
    char buffer[BUFFER_SIZE];
    int i = 0;
    while(i < BUFFER_SIZE - 1) {

        fd_set set;
         // clear the set and add our file descriptor to the set
        FD_ZERO(&set);               
        FD_SET(fileno(pipe), &set);  

        // 10 ms timeout to prevent blocking from select function
        struct timeval timeout;
        timeout.tv_sec = 0;
        timeout.tv_usec = 10000; 

        // select function to get the output from pipe
        // waits until file descriptor is ready for reading
        int relectResult = select(fileno(pipe) + 1, &set, NULL, NULL, &timeout);
        // return -1 if error
        if (relectResult == -1) {
            perror("select error"); 
            exit(1);
        } 
        // returns 0 if no data available
        else if (relectResult == 0) {
            break; 
        } 
        // data is available, read it
        else {
            // gets the input and put it in buffer
            char c = fgetc(pipe);
            buffer[i] = c;
            i++;

            // if it's a new line, add buffer to output
            if (c == '\n') {
                buffer[i] = '\0';
                strcat(output, buffer);
                break; 
            }
        }
    }

    return output;
}

// converts an int from midi input to enum note
enum note MidiReader_intToNote(int note) {
    for(int i = 0; i < NUM_OF_NOTES; i++) {
        if(notes[i].note_value == note) {
            // printf("note_map = %d, note = %d\n", notes[i].note_value, note);
            return notes[i].note;
        }
    }

    return NUM_OF_NOTES;
}

// converts a note to string for reading
char* MidiReader_noteToString(int note) {
    for(int i = 0; i < NUM_OF_NOTES; i++) {
        if(notes[i].note_value == note) {
            return notes[i].note_name;
        }
    }
    // default return for note out of range
    return "Unknown";
}

// gets and sets the note that need to be played
int MidiReader_getNoteToPlay(void){
    return noteToPlay;
}

void MidiReader_setNoteToPlay(enum note note) {
    noteToPlay = note;
}

// gets and sets variable indicating if a new note has been played
void MidiReader_setNeedToPlayNote(bool flag){
    needToPlayNote = flag;
}

bool MidiReader_getNeedToPlayNote() {
    return needToPlayNote;
}
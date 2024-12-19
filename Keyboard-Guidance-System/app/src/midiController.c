// module to control song played and check correctness
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdbool.h>

#include "midiController.h"
#include "parser.h"
#include "timeDelay.h"
#include "shutdown.h"
#include "hal/ledDriver.h"
#include "hal/colours.h"

#define MAX_NOTES 13

static pthread_t midiThread;
static wavedata_t sounds[MAX_NOTES];

// default start song
static int currentSong;
// get the current note that needs to be played from midi reader
static int noteToPlay;
// index to keep track of where in the txt file we are
static int currentNoteToPlayedIndex;
// flag to check if song has been set via joystick
static bool newSongSetFlag;
// array to keep track of current notes from txt file
static enum note *parsedNotes;
static StringArray parsedStringArray;

struct chord chord;

// notes properties
struct notes_map {
    enum note note;
    char* note_filepath;
    bool needToPlay;
};

// struct to keep track of notes and their status of needing to be played
static struct notes_map notes_map[] = {
    {C,       "piano_wave_sounds/c.wav",        false},
    {C_SHARP, "piano_wave_sounds/c_sharp.wav",  false},
    {D,       "piano_wave_sounds/d.wav",        false},
    {D_SHARP, "piano_wave_sounds/d_sharp.wav",  false},
    {E,       "piano_wave_sounds/e.wav",        false},
    {F,       "piano_wave_sounds/f.wav",        false},
    {F_SHARP, "piano_wave_sounds/f_sharp.wav",  false},
    {G,       "piano_wave_sounds/g.wav",        false},
    {G_SHARP, "piano_wave_sounds/g_sharp.wav",  false},
    {A,       "piano_wave_sounds/a.wav",        false},
    {A_SHARP, "piano_wave_sounds/a_sharp.wav",  false},
    {B,       "piano_wave_sounds/b.wav",        false},
    {C8,      "piano_wave_sounds/c8.wav",       false},
};

// songlist properties struct
struct songlist {
    enum songs songs;
    char* songPath;
    char* songName;
};

// struct to hold the list of songs
static struct songlist songList[] = {
    {TWINKLE,   "txt_files/twinkle_twinkle.txt",    "TWINKLE"},
    {BACH,      "txt_files/bach.txt",               "BACH"},
    {ZELDA,     "txt_files/zelda.txt",              "ZELDA"},
    {POKEMON,   "txt_files/pokemon.txt",            "POKEMON"},
    {BIRTHDAY,  "txt_files/birthday.txt",           "BIRTHDAY"},
    {POPSONG,   "txt_files/fun_song.txt",           "POPSONG"},
};

static void loadWaveFiles(void);
static void freeWaveFiles(void);

static char* getFilePath(enum note note) {
    for(int i = 0; i < NUM_OF_NOTES; i++) {
        if(notes_map[i].note == note) {
            return notes_map[i].note_filepath;
        }
    }
    return "";
}

static void* midiControllerthreadFunction() {
    // flag to keep of new notes to play
//    testNotesFunction();
    int playNote = 0;
    bool alreadyPlayed = false;
    // Clear all LEDs at the beginning of playBack in case there are some LEDs still on
    turnOffAllLEDs();

    // printf("First note to play: %s\n", MidiReader_noteToString(parsedNotes[currentNoteToPlayedIndex] + 60));
    while(!Shutdown_isShutdown()) {

        // if new song has been set
        if(newSongSetFlag){
            newSongSetFlag = false;
            turnOffAllLEDs();
        }

        // Clear previous note LED
        if (currentNoteToPlayedIndex != 0){
            changeColourLED(CLEAR, parsedNotes[currentNoteToPlayedIndex - 1]);
        }

        // Light up the current note to play
        changeColourLED(WHITE, parsedNotes[currentNoteToPlayedIndex]);

        // theres a new note from midi reader that needs to be played
        if (MidiReader_getNeedToPlayNote()) {
            // play it 
            noteToPlay = MidiReader_getNoteToPlay();
            audioGenerator_queueSound(&sounds[noteToPlay]);
            //Check if note to play is the same as the expected note

            // printf("Now play: %s\n", MidiReader_noteToString(parsedNotes[currentNoteToPlayedIndex] + 60));
            // Light up the current note to play
            if (noteToPlay == parsedNotes[currentNoteToPlayedIndex]){
                currentNoteToPlayedIndex++;
                printf("Note played correctly! Moving on to next note: %s\n", MidiReader_noteToString(parsedNotes[currentNoteToPlayedIndex] + 60));
                // printf("song index: %d, song size: %d\n", currentNoteToPlayedIndex, parsedStringArray.size);
                
                // If current note index exceed the buffer size, looping back
                // IE the song is finished playing
                if (currentNoteToPlayedIndex == parsedStringArray.size){
                    // reset it to whatever piece theyre playing right now
                    printf("Song finished! Looping back... \n");
                    LED_finishAnimation();
                    MidiController_setSong(currentSong);
                }
            } 
            // wrong note played, flash LEDs
            else {
                printf("Wrong note played! Please try again\n");
                printf("Expected note: %s\n", MidiReader_noteToString(parsedNotes[currentNoteToPlayedIndex] + 60));
                LED_triggerBlink(BRIGHT_RED, 13);
            }

            // set the flag to false
            MidiReader_setNeedToPlayNote(false);
        }

        sleepForMs(1);
    }
    return NULL;
}

// function to check if the note has been played correctly
bool MidiController_isNotePlayedCorrectly(enum note inputNote, enum note actualNote) {
    if(inputNote == actualNote) {            
        printf("New song, now playing: %s\n", songList[currentSong].songName);

        for(int i = 0; i < NUM_OF_NOTES; i++) {
            if(notes_map[i].note == inputNote) {
                notes_map[i].needToPlay = false;

            }
        }
        return true;
    }

    return false;
}

// set the note that's been played in a chord
void MidiController_setNotePlayed(enum note inputNote) {
    for(int i = 0; i < NUM_OF_NOTES; i++) {
        if(notes_map[i].note == inputNote) {
            notes_map[i].needToPlay = true;
            // printf("setNotePlayed = %d\n", notes_map[i].note);
        }
    }
}

// init and cleanup
void MidiController_init(void) {
    loadWaveFiles();

    printf("Press Joystick left or right to cycle through songs!\n");

    // prepare default song                    
    int defaultSong = TWINKLE;
    
    // printf("First song: %s\n", songList[defaultSong].songName);
    MidiController_setSong(defaultSong);

    newSongSetFlag = false;

    pthread_create(&midiThread, NULL, midiControllerthreadFunction, NULL);
}

void MidiController_cleanup(void) {
    pthread_join(midiThread, NULL);
    freeWaveFiles();
    printf("midi controller cleanup done!\n");

}

// load the ALSA wave files 
static void loadWaveFiles(void) {
    for (int i = 0; i < NUM_OF_NOTES; i++) {
        audioGenerator_readWaveFileIntoMemory(notes_map[i].note_filepath, &sounds[i]);
        printf("filename: %s\n", notes_map[i].note_filepath);
    }
    printf("\n");
}

static void freeWaveFiles(void) {
    for (int i = 0; i < NUM_OF_NOTES; i++) {
        audioGenerator_freeWaveFileData(&sounds[i]);
    }
}

void MidiController_setChord(struct chord inputChord) {
    chord = inputChord;
    // printf("Num of Notes: %d\n", chord.numNotes);

    // for(int i = 0; i < NUM_OF_NOTES; i++) {
        // printf("Current Note: %d\n", chord.currentNotes[i]);
    // }
}

int MidiController_getCurrentSong(){
    return currentSong;
}

// sets new song
void MidiController_setSong(int newSong){
    // get it as array from parser
    currentSong = newSong;
    parsedStringArray = Parser_parseFileByLineBreak(songList[currentSong].songPath);
    parsedNotes = Parse_parseStringArrayForNotes(parsedStringArray);
    // reset index
    currentNoteToPlayedIndex = 0;
    newSongSetFlag = true;
    printf("Song: %s\n", songList[currentSong].songName);
    printf("First note to play: %s\n", MidiReader_noteToString(parsedNotes[currentNoteToPlayedIndex] + 60));
}

void MidiController_cycleSongRight(){
    int newSong;
    // if it's at last song, cycle to first
    if (currentSong < NUM_SONGS - 1){
        newSong = currentSong + 1;
    }
    else {
        newSong = 0;
    }
    MidiController_setSong(newSong);
}

void MidiController_cycleSongLeft(){
    int newSong;
    // if it's at the first song, cycle to last song
    if (currentSong == 0){
        newSong = NUM_SONGS - 1;
    }
    else {
        newSong = currentSong - 1;
    }
    MidiController_setSong(newSong);
}
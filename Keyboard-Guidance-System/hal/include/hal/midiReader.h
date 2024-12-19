// Module to continuously read output from MIDI keyboard
// COnverts MIDI output into enum notes

#ifndef _MIDI_READER_H
#define _MIDI_READER_H

#include <bits/types/FILE.h>
#include <stdbool.h>

// notes represented as enum
enum note {
    C = 0,
    C_SHARP,
    D,
    D_SHARP,
    E,
    F,
    F_SHARP,
    G,
    G_SHARP,
    A,
    A_SHARP,
    B,
    C8, 
    NUM_OF_NOTES
};

// chord struct to hold multiple notes
struct chord {
    enum note* currentNotes;
    int numNotes;
    bool isNotePlayed;
};

// init and cleanup functions
void MidiReader_init(void);
void MidiReader_cleanup(void);

//converts int value to enum
enum note MidiReader_intToNote(int note);
// converts the enum note to string
char* MidiReader_noteToString(int note);

// gets the note that needs to be played
// to be called in src files
int MidiReader_getNoteToPlay();
void MidiReader_setNoteToPlay(enum note note);

// sets variable indicating if a new note has been played
// to be called in src files
bool MidiReader_getNeedToPlayNote();
void MidiReader_setNeedToPlayNote(bool flag);

#endif

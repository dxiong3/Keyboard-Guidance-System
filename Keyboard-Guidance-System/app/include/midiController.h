// Module to control the songs on display
// checks if correct notes are being played
// calls audio mixer to play sounds

#ifndef MIDICONTROLLER_H
#define MIDICONTROLLER_H

#include "hal/midiReader.h"
#include "hal/audioGenerator.h"

// songs represented as an enum
enum songs {
    TWINKLE = 0,
    BACH,
    ZELDA,
    POKEMON,
    BIRTHDAY,
    POPSONG,
    NUM_SONGS
};

// init and cleanup
void MidiController_init(void);
void MidiController_cleanup(void);

// checks if note is played correctly
bool MidiController_isNotePlayedCorrectly(enum note inputNote, enum note actualNote);

// checks if chords are being played correctly
void MidiController_setNotePlayed(enum note inputNote);
void MidiController_noteToPlay(int note);
void MidiController_setChord(struct chord chord);

// sets the new song
void MidiController_setSong(int newSong);
// cycle songs from joystick
void MidiController_cycleSongRight();
void MidiController_cycleSongLeft();
// gets the current song
int MidiController_getCurrentSong();

#endif

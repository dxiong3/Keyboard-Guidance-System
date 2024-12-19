#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h> //For isspace()

#include "parser.h"
#include "hal/midiReader.h"
#define MAX_LINES 500
#define MAX_LINE_LENGTH 100
#define MAX_CHORDS 100
#define MAX_NOTES 100

// converts a string representation of note to enum note
static enum note stringToNote(const char *str) {
    // Initialize with a default value
    enum note result = NUM_OF_NOTES; 
    
    // checks for sharp
    switch (str[0]) {
        case 'C':
            if (strcmp(str, "C8") == 0) result = C8;
            else if (str[1] == '#') result = C_SHARP;
            else result = C;
            break;
        case 'D':
            if (str[1] == '#') result = D_SHARP;
            else result = D;
            break;
        case 'E':
            result = E;
            break;
        case 'F':
            if (str[1] == '#') result = F_SHARP;
            else result = F;
            break;
        case 'G':
            if (str[1] == '#') result = G_SHARP;
            else result = G;
            break;
        case 'A':
            if (str[1] == '#') result = A_SHARP;
            else result = A;
            break;
        case 'B':
            result = B;
            break;
        default:
            break;
    }
    
    return result;
}

// ChatGPT generated function to remove any line break or white space character in a string
static void trimWhitespace(char *str) {
    char *end;

    // Trim leading whitespace
    while(isspace((unsigned char)*str))
        str++;

    // Trim trailing whitespace
    end = str + strlen(str) - 1;
    while(end > str && (isspace((unsigned char)*end) || *end == '\n')) // Also trim newline characters
        end--;

    // Write new null terminator
    *(end + 1) = '\0';
}

// creates a chord from a given line
static struct chord* createChord(enum note* notes, int numNotes) {
    struct chord* chord = (struct chord*)malloc(sizeof(struct chord));
    if (chord == NULL) {
        fprintf(stderr, "Memory allocation failed for chord.\n");
        return NULL;
    }

    chord->currentNotes = notes;
    chord->numNotes = numNotes;

    return chord;
}

// parse each txt file by new line 
// gets all the notes that need to be played in one line (chord or individual)
StringArray Parser_parseFileByLineBreak(char *filePath){
    // initialize variables
    FILE *file;
    char line[MAX_LINE_LENGTH];
    int index = 0;
    
    // open the file for reading
    file = fopen(filePath, "r");
    if (file == NULL) {
        fprintf(stderr, "Error opening file.\n");
    }

    // init StringArray to store each line
    StringArray lines;
    lines.size = MAX_LINES;
    lines.lines = (char**)malloc((lines.size + 1) * sizeof(char*));
   
   // initialize default value for each element of the lines array
    for (int i = 0; i < MAX_LINES + 1; i++) {
        lines.lines[i] = "";
    }
 
    // Read and parse lines until end of file
    while(fgets(line, sizeof(line), file)) {
        lines.lines[index] = malloc(strlen(line) + 1); // +1 for null terminator
    
        // get rid of space or line break characters(if any)
        trimWhitespace(line);
        strcpy(lines.lines[index], line);
        index++;
        lines.size = index;
    }

    // printf("lines size: %d\n", lines.size);

    // close the file
    fclose(file);
    return lines;
}

// take in an array of strings and map each string to its enum value
enum note* Parse_parseStringArrayForNotes(StringArray stringArray){
    // set up notes array to keep the notes
    enum note* notesArray = (enum note*)malloc((stringArray.size + 1) * sizeof(enum note));
    // printf("During init, the string array size is: %d\n", stringArray.size);

    // go through each element of the string array and convert raw string data to appropriate enum value
    for (int i = 0; i < stringArray.size; i++){
        notesArray[i] = stringToNote(stringArray.lines[i]);
    }
    // printf("After init notesArray, size: %d\n", notesArray.size);
    return notesArray;
}

// take in array of string, parse each element of the array by comma to seperate string 
// create a cord from each line of the text file
struct chord** Parser_parseStringArrayForChords(StringArray stringArray) {
    // Allocate memory for the array of chord pointers (Remember to clean up array of chord pointers when exit)
    struct chord** chords = (struct chord**)malloc((stringArray.size + 1) * sizeof(struct chord*));
    if (chords == NULL) {
        fprintf(stderr, "Memory allocation failed for chords array.\n");
        return NULL;
    }

    // free chords if notes array allocation fail
    for (int i = 0; i < stringArray.size; i++) {
        int numNotes = 0;
        // clean up when program is finished
        enum note* notesArray = (enum note*)malloc((stringArray.size + 1) * sizeof(enum note));
        if (notesArray == NULL) {
            fprintf(stderr, "Memory allocation failed.\n");
            // free previously allocated memory
            for (int j = 0; j < i; j++) {
                free(chords[j]);
            }
            free(chords);
            return NULL;
        }

        // parse a string by comma
        char *token = strtok(stringArray.lines[i], ","); 
        while(token != NULL) {
            // get rid of line break character (if any)
            trimWhitespace(token);  
            // convert string to enum
            enum note currentNote = stringToNote(token); 
            // only push if it is a valid note
            if (currentNote != NUM_OF_NOTES) {  
                notesArray[numNotes++] = currentNote; 
            }
            token = strtok(NULL, ",");
        }

        // create new chords out of notes in the same line
        struct chord* currentChord = createChord(notesArray, numNotes);
        chords[i] = currentChord; 
    }

    return chords;
}
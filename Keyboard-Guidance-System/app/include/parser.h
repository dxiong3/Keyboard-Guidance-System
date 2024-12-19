// module to parse notes from a txt file into array
// program will read the note line by line 
#ifndef _FILE_PARSER_H_
#define _FILE_PARSER_H_

//Store each line of the text file to each element of the lines array
typedef struct {
    char **lines;
    int size;
} StringArray;

// function to parse the file by line break
StringArray Parser_parseFileByLineBreak(char *);
// parse the file to get individual notes
enum note* Parse_parseStringArrayForNotes(StringArray);
// parse the file to set up chord structure
struct chord** Parser_parseStringArrayForChords(StringArray);

#endif
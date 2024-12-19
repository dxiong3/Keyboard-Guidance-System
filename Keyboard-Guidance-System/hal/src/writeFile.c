// Writes text to file on beaglebone

#include <stdio.h>
#include <stdlib.h>

void writeFile(const char * filePath, const char * text) {
    FILE * file;
    file = fopen(filePath, "w+");
    fprintf(file, "%s", text);
    fclose(file);
}
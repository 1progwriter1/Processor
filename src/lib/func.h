#ifndef FUNC_CALCULATOR
#define FUNC_CALCULATOR

#include <stdio.h>
#include "calculator_values.h"

FILE *fileopen(const char *filename, const char mode[]);
unsigned long filelen(const char *filename);
char *readfile(FILE *fn, unsigned long length);
int FileVerify(Header *filedata);
int DayNumber();
void fileclose(FILE *fn);

#endif

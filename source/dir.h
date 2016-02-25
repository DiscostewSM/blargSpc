#include <3ds.h>
#include <stdio.h>
#include <string.h>
#include <dirent.h>
#include <unistd.h>


// Initializes the directory set
//   When calling this, the default directory is the root directory
//   Cannot be initialized again until deinitialized
bool dirInit();

// DeInitializes the directory set
//   Cannot be deinitialized again until initialized
void dirDeInit();

void dirSetExt(char * extList);
bool dirReadDir(char * dirPath);
bool dirEnterDirectory(int idx);

int dirGetEntryCount();
bool dirIsEntryFile(int idx);
bool dirIsEntryDir(int idx);
char * dirGetEntry(int idx, bool alloc);
char * dirGetCurrentDir(bool alloc);
#ifndef FILE_H
#define FILE_H

#include "Desk.Save.h"


Desk_bool File_SaveGEDCOM(char *filename,void *ref);

void File_LoadGEDCOM(char *filename,Desk_bool plain);

void File_New(void);

void File_Remove(void);

void File_Modified(void);

Desk_bool File_GetModified(void);

char *File_GetDate(void);

char *File_GetFilename(void);

void File_Result(Desk_save_result result,void *ref);

#endif

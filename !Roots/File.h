#ifndef FILE_H
#define FILE_H

#ifndef __Desk_Save_h
#include "Desk.Save.h"
#endif

Desk_bool File_SaveFile(char *filename,void *ref);

void File_NewFile(void);

void File_Modified(void);

char *File_GetFilename(void);

void File_Result(Desk_save_result result,void *ref);

#endif

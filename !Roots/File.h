#ifndef FILE_H
#define FILE_H

#include "Desk.Save.h"

typedef enum tag {
	tag_WINDOW=0,
	tag_LAYOUT,
	tag_GRAPHICS,
	tag_DATABASE,
	tag_UNDOBUFFER
} tag;


Desk_bool File_SaveFile(char *filename,void *ref);

void File_LoadFile(char *filename);

void File_New(void);

void File_Remove(void);

void File_Modified(void);

Desk_bool File_GetModified(void);

char *File_GetDate(void);

int File_GetSize(void);

char *File_GetFilename(void);

void File_Result(Desk_save_result result,void *ref);

#endif

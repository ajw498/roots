#ifndef WINDOWS_H
#define WINDOWS_H

#ifndef DATABASE_H
#include "Database.h"
#endif

#ifndef LAYOUT_H
#include "Layout.h"
#endif

#define INFINITY 999999

#define EORCOLOUR 0xFFFFFF00
#define EORCOLOURRED 0xFFFF0000

typedef enum wintype {
	wintype_NORMAL,
	wintype_UNLINKED,
	wintype_DESCENDENTS,
	wintype_ANCESTORS,
	wintype_CLOSERELATIVES
} wintype;

void Windows_Init(void);

void Windows_Edit(void);

void Windows_ChangedLayout(void);

void Windows_OpenWindow(wintype type,elementptr person,int generations);

void Windows_Relayout(void);

int Windows_GetSize(void);

void Windows_Save(FILE *file);


#endif

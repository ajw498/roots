#ifndef WINDOWS_H
#define WINDOWS_H

#include "Database.h"
#include "Layout.h"

#define INFINITY 999999

#define EORCOLOUR 0xFFFFFF00
#define EORCOLOURRED 0xFFFF0000

typedef enum wintype {
	wintype_UNKNOWN=0,
	wintype_NORMAL,
	wintype_UNLINKED,
	wintype_DESCENDENTS,
	wintype_ANCESTORS,
	wintype_CLOSERELATIVES
} wintype;

void Windows_Init(void);

void Windows_Edit(void);

void Windows_ChangedLayout(void);

void Windows_ForceRedraw(void);

void Windows_CloseNewView(void);

Desk_bool Windows_BringToFront(void);

void Windows_OpenWindow(wintype type,elementptr person,int generations,int scale,Desk_convert_block *coords);

void Windows_Relayout(void);

int Windows_GetSize(void);

layout *Windows_Save(FILE *file,int *index);

void Windows_Load(FILE *file);

void Windows_FileModified(void);

void Windows_LayoutNormal(layout *layout,Desk_bool opencentred);

void Windows_CloseAddParentsWindow(void);

void Windows_CloseAllWindows(void);

#endif

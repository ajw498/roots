#ifndef GRAPHICS_H
#define GRAPHICS_H

#ifndef DATABASE_H
#include "Database.h"
#endif

#define INFINITY 999999

typedef enum wintype {
	wintype_NORMAL,
	wintype_UNLINKED,
	wintype_DESCENDENTS,
	wintype_ANCESTORS,
	wintype_CLOSERELATIVES
} wintype;

void Graphics_Init(void);

void Graphics_Edit(void);

void Graphics_ChangedLayout(void);

void Graphics_OpenWindow(wintype type,elementptr person,int generations);

void Graphics_Relayout(void);


#endif

#ifndef WINDOWS_H
#define WINDOWS_H

#include "DatabaseStruct.h"
#include "LayoutStruct.h"
#include "WindowsStruct.h"

#define INFINITY 999999

#define EORCOLOUR 0xFFFFFF00
#define EORCOLOURRED 0xFFFF0000

void Windows_Init(void);
void Windows_ChangedLayout(void);
void Windows_ForceRedraw(void);
Desk_bool Windows_Cancel(Desk_event_pollblock *block,void *ref);
void Windows_CloseNewView(void);
Desk_bool Windows_BringToFront(void);
void Windows_OpenWindow(wintype type,elementptr person,int generations,int scale,Desk_convert_block *coords);
void Windows_Relayout(void);
layout *Windows_SaveGEDCOM(FILE *file,int *index);
void Windows_FileModified(void);
void Windows_LayoutNormal(layout *layout,Desk_bool opencentred);
void Windows_CloseAllWindows(void);
void Windows_CreateWindow(wintype type);
void Windows_SetMinX(int val);
void Windows_SetMinY(int val);
void Windows_SetMaxX(int val);
void Windows_SetMaxY(int val);
void Windows_SetScrollX(int val);
void Windows_SetScrollY(int val);
void Windows_SetPerson(elementptr person);
void Windows_SetGenerations(int val);
void Windows_SetScale(int val);
void Windows_SetUpMenu(windowdata *windowdata,elementtype selected,int x,int y);
void Windows_GraphicsStylesMenu(Desk_menu_ptr *menuptr,char *dirname);

#endif

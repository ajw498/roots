#ifndef LAYOUT_H
#define LAYOUT_H

#include "LayoutStruct.h"
#include "DatabaseStruct.h"
#include "WindowsStruct.h"

#define MINWINDOW_SIZE_X 500
#define MINWINDOW_SIZE_Y 256

extern mouseclickdata mousedata;

void Layout_ChangeMarriageTypes(layout *layout,Desk_bool separate);
void Layout_DeleteSelected(layout *layout);
void Layout_UnlinkSelected(layout *layout);
Desk_bool Layout_MouseClick(Desk_event_pollblock *block,void *ref);
void Layout_ResizeWindow(windowdata *windowdata);
Desk_bool Layout_RedrawWindow(Desk_event_pollblock *block,windowdata *windowdata);
void Layout_Redraw(layout *layout,int scale,int originx,int originy,Desk_wimp_box *cliprect,Desk_bool plotselection);
int Layout_NearestGeneration(int y);
layout *Layout_New(void);
void Layout_Free(layout *layout);
Desk_wimp_rect Layout_FindExtent(layout *layout,Desk_bool selection);
void Layout_AddElement(layout *layout,elementptr person,int x,int y,int width,int height,int xgrid,int ygrid,flags flags);
void Layout_AddTransient(layout *layout,elementptr person,int x,int y,int width,int height,int xgrid,int ygrid,flags flags);
void Layout_SetWidth(layout *layout,elementptr person,int width);
int Layout_FindXCoord(layout *layout,elementptr person);
int Layout_FindYCoord(layout *layout,elementptr person);
int Layout_FindWidth(layout *layout,elementptr person);
int Layout_FindHeight(layout *layout,elementptr person);
int Layout_FindXGridCoord(layout *layout,elementptr person);
int Layout_FindYGridCoord(layout *layout,elementptr person);
void Layout_RemoveElement(layout *layout,elementptr person);
void Layout_Select(elementptr person);
void Layout_DeSelect(elementptr person);
void Layout_DeSelectAll(void);
Desk_bool Layout_GetSelect(elementptr person);
elementtype Layout_AnyoneSelected(void);
void Layout_Init(void);
void Layout_RemoveTransients(layout *layout);
void Layout_CalcAllGridFromPositions(layout *layout);
void Layout_CalcAllPositionsFromGrid(layout *layout);
void Layout_ResizeAllWidths(layout *layout);

#endif


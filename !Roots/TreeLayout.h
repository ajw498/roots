#ifndef TREELAYOUT_H
#define TREELAYOUT_H

#include "Layout.h"


layout *Layout_LayoutNormal(void);
layout *Layout_LayoutDescendents(elementptr person, int generations);
layout *Layout_LayoutAncestors(elementptr person, int generations);
void Layout_LayoutLines(layout *layout,Desk_bool layoutseparatemarriages);
void Layout_LayoutTitle(layout *layout);
void TreeLayout_AddMarriage(layout *layout,elementptr marriage);
void Layout_SelectDescendents(layout *layout,elementptr person);
void Layout_SelectAncestors(layout *layout,elementptr person);
void Layout_SelectSiblings(layout *layout,elementptr person);
void Layout_SelectSpouses(layout *layout,elementptr person);
void Layout_SaveGEDCOM(layout *layout,FILE *file);
layout *Layout_GetGEDCOMLayout(void);
void Layout_GEDCOMNewPerson(elementptr person);
void Layout_GEDCOMNewPersonX(int pos);
void Layout_GEDCOMNewPersonY(int pos);
void Layout_GEDCOMNewPersonWidth(int width);
void Layout_GEDCOMNewPersonHeight(int height);
void Layout_GEDCOMNewPersonFlags(flags flags);
void TreeLayout_CheckForUnlink(layout *layout,elementptr i);

#endif


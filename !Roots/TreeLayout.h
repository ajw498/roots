#ifndef TREELAYOUT_H
#define TREELAYOUT_H

#include "Layout.h"


layout *Layout_LayoutNormal(void);
layout *Layout_LayoutDescendents(elementptr person, int generations);
layout *Layout_LayoutAncestors(elementptr person, int generations);
void Layout_LayoutMarriages(layout *layout);
void Layout_LayoutLines(layout *layout);
void Layout_LayoutTitle(layout *layout);
void Layout_SelectDescendents(layout *layout,elementptr person);
void Layout_SelectAncestors(layout *layout,elementptr person);
void Layout_SelectSiblings(layout *layout,elementptr person);
void Layout_SelectSpouses(layout *layout,elementptr person);
void Layout_SaveGEDCOM(layout *layout,FILE *file);
layout *Layout_GetGEDCOMLayout(void);
void Layout_GEDCOMNewPerson(elementptr person);
void Layout_GEDCOMNewPersonX(int pos);
void Layout_GEDCOMNewPersonY(int pos);
void Layout_GEDCOMNewMarriage(elementptr marriage);
void Layout_GEDCOMNewMarriageX(int pos);
void Layout_GEDCOMNewMarriageY(int pos);

#endif


#ifndef LAYOUT_H
#define LAYOUT_H

#ifndef ELEMENTPTR
#define ELEMENTPTR

typedef int elementptr;

#endif

typedef struct personlayout {
	int x;
	int y;
	int generation;
	elementptr person;
} personlayout;

typedef struct marriagelayout {
	int x;
	int y;
	elementptr marriage;
} marriagelayout;

typedef struct childlinelayout {
	int leftx;
	int rightx;
	int y;
} childlinelayout;

typedef struct titlelayout {
	int x;
	int y;
} titlelayout;

typedef struct layout {
	titlelayout title;
	personlayout *person;
	int numpeople;
	marriagelayout *marriage;
	int nummarriages;
	childlinelayout *children;
	int numchildren;
} layout;

layout *Layout_LayoutNormal(void);
layout *Layout_LayoutDescendents(elementptr person, int generations);
layout *Layout_LayoutAncestors(elementptr person, int generations);
int Layout_NearestGeneration(int y);
void Layout_Free(layout *layout);
Desk_wimp_rect Layout_FindExtent(layout *layout,Desk_bool selection);
void Layout_AlterChildline(layout *layout,elementptr person,Desk_bool on);
void Layout_AlterMarriageChildline(layout *layout,elementptr marriage,Desk_bool on);
void Layout_AddPerson(layout *layout,elementptr person,int x,int y);
void Layout_AddMarriage(layout *layout,elementptr marriage,int x,int y);
int Layout_FindXCoord(layout *layout,elementptr person);
int Layout_FindYCoord(layout *layout,elementptr person);
int Layout_FindMarriageXCoord(layout *layout,elementptr marriage);
int Layout_FindMarriageYCoord(layout *layout,elementptr marriage);
void Layout_LayoutMarriages(layout *layout);
void Layout_LayoutLines(layout *layout);
void Layout_LayoutTitle(layout *layout);
void Layout_SelectDescendents(layout *layout,elementptr person);
void Layout_SelectAncestors(layout *layout,elementptr person);
void Layout_SelectSiblings(layout *layout,elementptr person);
void Layout_SelectSpouses(layout *layout,elementptr person);
void Layout_RemovePerson(layout *layout,elementptr person);
void Layout_RemoveMarriage(layout *layout,elementptr marriage);
void Layout_SaveGEDCOM(layout *layout,FILE *file);
layout *Layout_GetGEDCOMLayout(void);
void Layout_GEDCOMNewPerson(elementptr person);
void Layout_GEDCOMNewPersonX(int pos);
void Layout_GEDCOMNewPersonY(int pos);
void Layout_GEDCOMNewMarriage(elementptr marriage);
void Layout_GEDCOMNewMarriageX(int pos);
void Layout_GEDCOMNewMarriageY(int pos);

#endif


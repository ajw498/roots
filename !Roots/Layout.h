#ifndef LAYOUT_H
#define LAYOUT_H

#ifndef ELEMENTPTR
#define ELEMENTPTR

typedef int elementptr;

typedef enum elementtype {
	element_NONE,
	element_PERSON,
	element_MARRIAGE,
	element_SELECTION,
	element_FREE,
	element_FILE,
	element_TITLE
} elementtype;

#endif

typedef struct elementlayout {
	int x;
	int y;
	int xgrid;
	int ygrid;
	elementptr element;
	int width;
	int height;
	struct {
		unsigned int editable   : 1;
		unsigned int moveable   : 1;
		unsigned int linkable   : 1;
		unsigned int snaptogrid : 1;
		unsigned int selectable : 1;
		unsigned int transient  : 1;
	} flags;
} elementlayout;

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
	elementlayout *person;
	int numpeople;
	elementlayout *marriage;
	int nummarriages;
	childlinelayout *children;
	int numchildren;
} layout;

int Layout_NearestGeneration(int y);
void Layout_Free(layout *layout);
Desk_wimp_rect Layout_FindExtent(layout *layout,Desk_bool selection);
void Layout_AddPerson(layout *layout,elementptr person,int x,int y);
void Layout_AddMarriage(layout *layout,elementptr marriage,int x,int y);
int Layout_FindXCoord(layout *layout,elementptr person);
int Layout_FindYCoord(layout *layout,elementptr person);
int Layout_FindMarriageXCoord(layout *layout,elementptr marriage);
int Layout_FindMarriageYCoord(layout *layout,elementptr marriage);
void Layout_RemovePerson(layout *layout,elementptr person);
void Layout_RemoveMarriage(layout *layout,elementptr marriage);
void Layout_Select(elementptr person);
void Layout_DeSelect(elementptr person);
void Layout_DeSelectAll(void);
Desk_bool Layout_GetSelect(elementptr person);
elementtype Layout_AnyoneSelected(void);

#endif


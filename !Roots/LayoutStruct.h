#ifndef LAYOUT_H
#define LAYOUT_H

#ifndef ELEMENTPTR
#define ELEMENTPTR

typedef int elementptr;

typedef enum elementtype {
	element_TITLE=-5,
	element_CHILDLINE,
	element_NONE=0,
	element_PERSON,
	element_MARRIAGE,
	element_SELECTION,
	element_FREE,
	element_FILE
} elementtype;

#endif

typedef struct flags {
	unsigned int editable   : 1;
	unsigned int moveable   : 1;
	unsigned int linkable   : 1;
	unsigned int snaptogrid : 1;
	unsigned int selectable : 1;
} flags;

typedef struct elementlayout {
	int x;
	int y;
	int xgrid;
	int ygrid;
	elementptr element;
	int width;
	int height;
	flags flags;
} elementlayout;

typedef struct titlelayout {
	int x;
	int y;
} titlelayout;

typedef struct layout {
	titlelayout title;
	elementlayout *person;
	int numpeople;
	elementlayout *transients;
	int numtransients;
	int gridx;
	int gridy;
	flags flags;
} layout;

typedef struct mouseclickdata {
	struct windowdata *window;
	elementptr element;
	Desk_wimp_point pos;
	Desk_bool transient;
} mouseclickdata;

extern mouseclickdata mousedata;

void Layout_Redraw(layout *layout,int scale,int originx,int originy,Desk_wimp_box *cliprect,Desk_bool plotselection);
int Layout_NearestGeneration(int y);
layout *Layout_New(void);
void Layout_Free(layout *layout);
Desk_wimp_rect Layout_FindExtent(layout *layout,Desk_bool selection);
void Layout_AddElement(layout *layout,elementptr person,int x,int y,int width,int height,int xgrid,int ygrid,flags flags);
void Layout_AddTransient(layout *layout,elementptr person,int x,int y,int width,int height,int xgrid,int ygrid,flags flags);
int Layout_FindXCoord(layout *layout,elementptr person);
int Layout_FindYCoord(layout *layout,elementptr person);
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

#endif


#ifndef GRAPHICS_H
#define GRAPHICS_H

#include "Desk.Font2.h"

#include "Layout.h"

#define NUMPERSONFIELDS 20
#define NUMMARRIAGEFIELDS 20 /*these are not correct*/

typedef enum personfieldtype {
	personfieldtype_SURNAME=0,
	personfieldtype_FORENAME,
	personfieldtype_MIDDLENAMES,
	personfieldtype_TITLE,
	personfieldtype_FULLNAME,
	personfieldtype_NAME,
	personfieldtype_TITLEDNAME,
	personfieldtype_TITLEDFULLNAME,
	personfieldtype_SEX,
	personfieldtype_DOB,
	personfieldtype_DOD,
	personfieldtype_BIRTHPLACE,
	personfieldtype_USER1,
	personfieldtype_USER2,
	personfieldtype_USER3
} personfieldtype;

typedef enum marriagefieldtype {
	marriagefieldtype_PLACE=0,
	marriagefieldtype_DATE,
	marriagefieldtype_DIVORCE
} marriagefieldtype;

typedef struct textproperties {
	unsigned int colour;
	unsigned int bgcolour;
	int x;
	int y;
	char fontname[256];
	int size;
	Desk_font2_block *font;
} textproperties;

typedef struct lineboxproperties {
	int x0;
	int y0;
	int x1;
	int y1;
	int colour;
	int thickness;
} lineboxproperties;

typedef enum graphictype {
	graphictype_LINE,
	graphictype_CHILDLINE,
	graphictype_SIBLINGLINE,
	graphictype_RECTANGLE,
	graphictype_FILLEDRECTANGLE,
	graphictype_TEXTLABEL,
	graphictype_CENTREDTEXTLABEL,
	graphictype_FIELD,
	graphictype_CENTREDFIELD,
	graphictype_INVALID
} graphictype;

typedef struct textlabel {
	textproperties properties;
	char text[256];
} textlabel;

typedef union details {
	textlabel textlabel;
	lineboxproperties linebox;
} details;

typedef struct object {
	graphictype type;
	details details;
} object;

typedef struct fieldproperties {
	Desk_bool plot;
    graphictype type;
	textproperties textproperties;
} fieldproperties;

typedef struct graphics {
	int personwidth;
	int personheight;
	int gapheightabove;
	int gapheightbelow;
	int gapwidth;
	int marriagewidth;
	int secondmarriagegap;
	int windowborder;
	int gapheightunlinked;
	int siblinglinethickness;
	int siblinglinecolour;
	int titleheight;
	textproperties title;
	int numpersonobjects;
	object *person;
	int nummarriageobjects;
	object *marriage;
	fieldproperties personfields[NUMPERSONFIELDS];
	fieldproperties marriagefields[NUMMARRIAGEFIELDS];
} graphics;

typedef void (*plotfn)(const int scale,const int originx,const int originy,const int minx,const int miny,const int maxx,const int maxy,const int linethickness,const unsigned int colour);
typedef void (*plottextfn)(const int scale,const int originx,const int originy,const int x,const int y,const int handle,const char *font,const int size,const unsigned int bgcolour,const unsigned int fgcolour,const char *text);

int Graphics_PersonHeight(void);

int Graphics_PersonWidth(void);

int Graphics_UnlinkedGapHeight(void);

int Graphics_GapHeightAbove(void);

int Graphics_GapHeightBelow(void);

int Graphics_GapWidth(void);

int Graphics_MarriageWidth(void);

int Graphics_SecondMarriageGap(void);

int Graphics_WindowBorder(void);

int Graphics_TitleHeight(void);

void Graphics_Redraw(layout *layout,int scale,int originx,int originy,Desk_wimp_box *cliprect,Desk_bool plotselection,plotfn plotline,plotfn plotrect,plotfn plotrectfilled,plottextfn plottext);

void Graphics_PlotPerson(int scale,int originx,int originy,elementptr person,int x,int y,Desk_bool child,Desk_bool selected);

int Graphics_GetSize(void);

void Graphics_Load(FILE *file);

void Graphics_Save(FILE *file);

void Graphics_RemoveStyle(void);

void Graphics_LoadStyle(char *style);

#endif

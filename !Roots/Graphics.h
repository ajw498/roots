#ifndef GRAPHICSCONFIG_H
#define GRAPHICSCONFIG_H

#include "Desk.Font2.h"


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
	marriagefieldtype_DATE
	/*etc*/
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
	int numpersonobjects;
	object *person;
	int nummarriageobjects;
	object *marriage;
	fieldproperties personfields[NUMPERSONFIELDS];
	fieldproperties marriagefields[NUMMARRIAGEFIELDS];
} graphics;

void Graphics_StoreDimensionDetails(char *values[],int numvalues,int linenum);

void Graphics_StorePersonDetails(char *values[],int numvalues,int linenum);

void Graphics_StoreMarriageDetails(char *values[],int numvalues,int linenum);

void Graphics_ReadFile(char *filename,void (*decodefn)(char *values[],int numvalues,int linenum));

int Graphics_PersonHeight(void);

int Graphics_PersonWidth(void);

int Graphics_UnlinkedGapHeight(void);

int Graphics_GapHeightAbove(void);

int Graphics_GapHeightBelow(void);

int Graphics_GapWidth(void);

int Graphics_MarriageWidth(void);

int Graphics_SecondMarriageGap(void);

int Graphics_WindowBorder(void);

void Graphics_Redraw(layout *layout,int originx,int originy,Desk_wimp_box *cliprect,Desk_bool plotselection,plotfn plotline,plotfn plotrect,plotfn plotrectfilled,plottextfn plottext);

void Graphics_PlotPerson(elementptr person,int x,int y,Desk_bool child,Desk_bool selected);

void Graphics_Init(void);

#endif

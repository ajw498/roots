/*
	Roots - Tree related layout routines
	© Alex Waugh 1999

	$Id: TreeLayout.c,v 1.42 2000/10/13 19:25:53 AJW Exp $

*/

#include "Desk.Core.h"
#include "Desk.Error2.h"
#include "Desk.Window.h"
#include "Desk.Event.h"
#include "Desk.DeskMem.h"

#include "AJWLib.Flex.h"
#include "AJWLib.File.h"
#include "AJWLib.Assert.h"

#include <stdlib.h>
#include <stdio.h>

#include "Database.h"
#include "Graphics.h"
#include "Modules.h"
#include "Windows.h"
#include "TreeLayout.h"
#include "Layout.h"
#include "File.h"
#include "Config.h"


#define LARGENUMBERINCREMENT 10000

typedef struct {
	int minx;
	int maxx;
} line;

typedef enum direction {
	direction_RTOL,
	direction_BOTH,
	direction_LTOR,
	direction_NONE
} direction;

typedef void (*callfn)(layout *layout,elementptr person,int);

static line *spaces;
static int mingeneration,maxgeneration;
/*static int numgenerations,addamount,firstplot,largenumber;*/
/*static Desk_bool selectmarriages;*/
static layout *gedcomlayout=NULL;

/*static void Layout_TraverseTree(layout *layout,elementptr person,int domarriage,int doallsiblings,Desk_bool dochild,Desk_bool doparents,Desk_bool lefttoright,int generation,callfn fn);
static void Layout_TraverseAncestorTree(layout *layout,elementptr person,int generation,callfn fn);*/


void Layout_LayoutTitle(layout *layout)
{
	Desk_wimp_rect bbox;
	AJWLib_Assert(layout!=NULL);
	if (!Config_Title()) return;
	if (layout->title.x==INFINITY && layout->title.y==INFINITY) return;
	bbox=Layout_FindExtent(layout,Desk_FALSE);
	layout->title.x=(bbox.max.x+bbox.min.x)/2;
	layout->title.y=bbox.max.y+Graphics_TitleHeight()/2;
}

static void Layout_ExtendGeneration(int generation)
{
	int i;
	if (generation<mingeneration) {
		AJWLib_Flex_MidExtend((flex_ptr)&spaces,0,sizeof(line)*(mingeneration-generation));
		for (i=0;i<mingeneration-generation;i++) {
			spaces[i].minx=0;
			spaces[i].maxx=0;
		}
		mingeneration=generation;
	}
	if (generation>maxgeneration) {
		AJWLib_Flex_Extend((flex_ptr)&spaces,sizeof(line)*(generation-mingeneration+1));
		for (i=maxgeneration-mingeneration+1;i<generation-mingeneration+1;i++) {
			spaces[i].minx=0;
			spaces[i].maxx=0;
		}
		maxgeneration=generation;
	}
}

static void Layout_PlotChildLine(layout *layout,elementptr person,int y)
{
	AJWLib_Assert(layout!=NULL);
	AJWLib_Assert(person!=none);

	if (Database_GetSiblingRtoL(person)==none) {
		/*Only plot the line if we are the leftmost sibling*/
		elementptr marriage;
		if ((marriage=Database_GetParentsMarriage(person))!=none) {
			/*Only plot line if the person has parents*/
			int leftpos=INFINITY,rightpos=-INFINITY,marriagepos=INFINITY;
			do {
				/*Find leftmost and rightmost positions of siblings*/
				int pos;
				pos=Layout_FindXCoord(layout,person);
				if (pos<leftpos) leftpos=pos;
				if (pos>rightpos) rightpos=pos;
			} while ((person=Database_GetSiblingLtoR(person))!=none);
			leftpos+=Graphics_PersonWidth()/2;
			rightpos+=Graphics_PersonWidth()/2;
			/*See if parents marriage is further left or right than the siblings*/
			marriagepos=Layout_FindMarriageXCoord(layout,marriage)+Graphics_MarriageWidth()/2;
			if (marriagepos<leftpos) leftpos=marriagepos;
			if (marriagepos>rightpos) rightpos=marriagepos;
			/*Create the line*/
			AJWLib_Flex_Extend((flex_ptr)&(layout->children),sizeof(childlinelayout)*(layout->numchildren+1));
			layout->children[layout->numchildren].rightx=rightpos;
			layout->children[layout->numchildren].leftx=leftpos;
			layout->children[layout->numchildren].y=y;
			layout->numchildren++;
		}
	}
}

static void Layout_PlotPerson(layout *layout,elementptr person,int generation)
{
	elementptr marriage;

	AJWLib_Assert(layout!=NULL);
	AJWLib_Assert(person!=none);
	marriage=Database_GetMarriage(person);
	AJWLib_Flex_Extend((flex_ptr)&(layout->person),sizeof(elementlayout)*(layout->numpeople+1));
	layout->numpeople++; /*Only incremented if there was no error*/
	if (marriage && Database_GetPrincipalFromMarriage(marriage)!=person) {
		spaces[generation-mingeneration].maxx+=Graphics_MarriageWidth();
		if (!Database_IsFirstMarriage(marriage)) spaces[generation-mingeneration].maxx+=Graphics_SecondMarriageGap();
	} else {
		spaces[generation-mingeneration].maxx+=Graphics_GapWidth();
	}
	layout->person[layout->numpeople-1].x=spaces[generation-mingeneration].maxx;
	spaces[generation-mingeneration].maxx+=Graphics_PersonWidth();
	layout->person[layout->numpeople-1].y=Layout_NearestGeneration((generation)*-(Graphics_GapHeightAbove()+Graphics_GapHeightBelow()+Graphics_PersonHeight()));
	layout->person[layout->numpeople-1].ygrid=generation;
	layout->person[layout->numpeople-1].element=person;
#ifdef DEBUG
halt=Desk_TRUE;
debuglayout=layout;
while (halt) Desk_Event_Poll();
Desk_Window_ForceRedraw(-1,0,0,100000,100000);
#endif
}

static void Layout_PlotBodgedMarriages(layout *layout)
/* Plot all unplotted marriages on the correct generation, but at the far right of the tree*/
{
	int index=0;

	AJWLib_Assert(layout!=NULL);

	do {
		elementptr marriage=Database_GetLinkedMarriages(&index);

		if (marriage) {
			if (!Layout_GetSelect(marriage)) {
				int generation=0,i;

				for (i=0;i<layout->numpeople;i++) if (layout->person[i].element==Database_GetPrincipalFromMarriage(marriage)) generation=layout->person[i].ygrid;
			
				AJWLib_Flex_Extend((flex_ptr)&(layout->marriage),sizeof(elementlayout)*(layout->nummarriages+1));
				layout->nummarriages++;
				layout->marriage[layout->nummarriages-1].x=spaces[generation-mingeneration].maxx+Graphics_SecondMarriageGap();
				spaces[generation-mingeneration].maxx+=Graphics_MarriageWidth()+Graphics_SecondMarriageGap();
				layout->marriage[layout->nummarriages-1].y=Layout_NearestGeneration((generation)*-(Graphics_GapHeightAbove()+Graphics_GapHeightBelow()+Graphics_PersonHeight()));
				layout->marriage[layout->nummarriages-1].element=marriage;
				Layout_Select(marriage);
			}
		}
	} while (index);
}

static void Layout_PlotMarriage(layout *layout,elementptr person,int x,int y)
{
	elementptr marriage;

	AJWLib_Assert(layout!=NULL);
	AJWLib_Assert(person!=none);
	
	marriage=Database_GetMarriage(person);
	if (marriage==none || Database_GetPrincipalFromMarriage(marriage)==person) return;
	if (Layout_GetSelect(marriage)) return;
	x-=Graphics_MarriageWidth();
	AJWLib_Flex_Extend((flex_ptr)&(layout->marriage),sizeof(elementlayout)*(layout->nummarriages+1));
	layout->nummarriages++;
	layout->marriage[layout->nummarriages-1].x=x;
	layout->marriage[layout->nummarriages-1].y=y;
	layout->marriage[layout->nummarriages-1].element=marriage;
	Layout_Select(marriage);
}

static int Layout_FindChildCoords(layout *layout,elementptr marriage)
{
	elementptr leftchild,rightchild;
	int i,leftx=0,rightx=0; /*a better method of error checking?*/
	AJWLib_Assert(layout!=NULL);
	AJWLib_Assert(marriage!=none);
	leftchild=Database_GetLeftChild(marriage);
	rightchild=leftchild;
	while (Database_GetSiblingLtoR(rightchild)!=none) rightchild=Database_GetSiblingLtoR(rightchild);
	for (i=0;i<layout->numpeople;i++) {
		if (layout->person[i].element==leftchild) leftx=layout->person[i].x;
		if (layout->person[i].element==rightchild) rightx=layout->person[i].x;
		/*optimise this loop*/
	}
	return (leftx+rightx+Graphics_PersonWidth())/2;
}

/*static void Layout_Add(layout *layout,elementptr person,int generation,Desk_bool dummy1,Desk_bool dummy2)
{
	int i,amount;
	elementptr marriage;
	AJWLib_Assert(layout!=NULL);
	AJWLib_Assert(person!=none);
	Desk_UNUSED(dummy1);
	Desk_UNUSED(dummy2);
	marriage=Database_GetMarriage(person);
	for (i=0;i<layout->numpeople;i++) {
		if (layout->person[i].element==person) {
			if (addamount<0 && spaces[generation-mingeneration].maxx==layout->person[i].x+Graphics_PersonWidth()) spaces[generation-mingeneration].maxx+=addamount;
			if (addamount>0 && spaces[generation-mingeneration].maxx==layout->person[i].x+Graphics_PersonWidth()) spaces[generation-mingeneration].maxx+=addamount;
			if (marriage && Database_GetPrincipalFromMarriage(marriage)!=person) {
				amount=Graphics_MarriageWidth();
				if (!Database_IsFirstMarriage(marriage)) amount+=Graphics_SecondMarriageGap();
			} else {
				amount=Graphics_GapWidth();
			}
			if (addamount>0 && spaces[generation-mingeneration].minx==layout->person[i].x-amount) spaces[generation-mingeneration].minx+=addamount;
			if (addamount<0 && spaces[generation-mingeneration].minx==layout->person[i].x-amount) spaces[generation-mingeneration].minx+=addamount;
			layout->person[i].x+=addamount;
		}
		*optimise this loop*
	}
}

static void Layout_Select(layout *layout,elementptr person,int dummy0,Desk_bool dummy1,Desk_bool dummy2)
{
	elementptr marriage;
	AJWLib_Assert(layout!=NULL);
	AJWLib_Assert(person!=none);
	Desk_UNUSED(dummy0);
	Desk_UNUSED(dummy1);
	Desk_UNUSED(dummy2);
	Database_Select(person);
	marriage=Database_GetMarriage(person);
	if (Database_GetPrincipalFromMarriage(marriage)!=person) {
		Database_Select(marriage);
	}
}*/

static void Layout_Plot(layout *layout,elementptr person,int generation)
{
	elementptr marriage;

	AJWLib_Assert(layout!=NULL);
	AJWLib_Assert(person!=none);

	/* Make sure we have set up structure for this generation*/
	Layout_ExtendGeneration(generation);

/*	if (firstplot==Desk_FALSE && spaces[generation-mingeneration].minx==spaces[generation-mingeneration].maxx) {
		if (lefttoright) {
			spaces[generation-mingeneration].minx=-(largenumber+=LARGENUMBERINCREMENT);
			spaces[generation-mingeneration].maxx=-largenumber;
		} else {
			spaces[generation-mingeneration].minx=(largenumber+=LARGENUMBERINCREMENT);
			spaces[generation-mingeneration].maxx=largenumber;
		}
	}*/
/*	if (firstplot==Desk_TRUE) firstplot=Desk_FALSE;*/
	marriage=Database_GetMarriage(person);
	if (marriage) {
		if (Database_GetLeftChild(marriage)!=none) {
			int childcoords;
			childcoords=Layout_FindChildCoords(layout,marriage);
			if (Database_GetMarriageRtoL(person)!=Database_GetPrincipalFromMarriage(marriage)) {
				int maxx=spaces[generation-mingeneration].maxx+Graphics_MarriageWidth()/2;
				if (Database_GetPrincipalFromMarriage(marriage)==person) maxx+=Graphics_PersonWidth()+Graphics_GapWidth();
                if (!Database_IsFirstMarriage(marriage)) maxx+=Graphics_SecondMarriageGap();
				if (childcoords>=maxx) {
					spaces[generation-mingeneration].maxx+=childcoords-maxx;
				} else {
					int newmaxx;
/*					addamount=maxx-childcoords;
					Layout_TraverseNormalTree(layout,Database_GetRightChild(marriage),2,2,Desk_TRUE,Desk_FALSE,Desk_FALSE,generation+1,Layout_Add);*/
					newmaxx=spaces[generation-mingeneration].maxx+Graphics_MarriageWidth()/2;
					if (Database_GetPrincipalFromMarriage(marriage)==person) newmaxx+=Graphics_PersonWidth()+Graphics_GapWidth();
					if (!Database_IsFirstMarriage(marriage)) newmaxx+=Graphics_SecondMarriageGap();
					if (newmaxx!=maxx) {
/*						addamount=maxx-newmaxx;
						Layout_TraverseTree(layout,Database_GetRightChild(marriage),2,2,Desk_TRUE,Desk_FALSE,Desk_FALSE,generation+1,Layout_Add);*/
					}
				}
			}
		}
	}
	Layout_PlotPerson(layout,person,generation);
}

/*static void Layout_PlotAncestors(layout *layout,elementptr person,int generation,Desk_bool lefttoright,Desk_bool child)
{
	int amount=0;
	elementptr mother,principal,principalmother;
	int motherx,principalmotherx;
	AJWLib_Assert(layout!=NULL);
	AJWLib_Assert(person!=none);
	Layout_ExtendGeneration(generation);
	Layout_ExtendGeneration(generation-1);
	mother=Database_GetMother(person);
	principal=Database_GetPrincipalFromMarriage(Database_GetMarriage(person));
	principalmother=Database_GetMother(principal);
	motherx=Layout_FindXCoord(layout,mother);
	principalmotherx=Layout_FindXCoord(layout,principalmother);
	if (person!=principal) {
		if (mother && principalmother) {
			amount=(motherx+principalmotherx)/2+Graphics_PersonWidth();
		} else if (mother || principalmother) {
			amount=motherx+principalmotherx+(Graphics_PersonWidth()-Graphics_MarriageWidth())/2;
		}
		if (amount<spaces[generation-mingeneration].minx) {
			spaces[generation-mingeneration].minx=amount;
		} else {
			addamount=spaces[generation-mingeneration].minx-amount;
			Layout_TraverseAncestorTree(layout,mother,generation-1,Layout_Add);
		}
	} else {
		amount=principalmotherx+(Graphics_PersonWidth()-Graphics_MarriageWidth())/2;
		if (amount>spaces[generation-mingeneration].minx) {
			addamount=spaces[generation-mingeneration].minx-principalmotherx-Graphics_PersonWidth()-(Graphics_GapWidth()-Graphics_MarriageWidth())/2;
			*This gets it wrong if there is no mother but the principal mother is too far to the right*
			Layout_TraverseAncestorTree(layout,principalmother,generation-1,Layout_Add);
		}
	}
	Layout_PlotPerson(layout,person,generation,lefttoright,child);
}*/


/*static void Layout_TraverseTree(layout *layout,elementptr person,int domarriage,int doallsiblings,Desk_bool dochild,Desk_bool doparents,Desk_bool lefttoright,int generation,callfn fn)
{
	AJWLib_Assert(layout!=NULL);
	if (person==none) return;
	if (domarriage>=2 && lefttoright)  Layout_TraverseTree(layout,Database_GetMarriageRtoL(person),3,2,Desk_TRUE,Desk_TRUE,lefttoright,generation,fn);
	if (domarriage>=2 && !lefttoright) Layout_TraverseTree(layout,Database_GetMarriageLtoR(person),3,2,Desk_TRUE,Desk_TRUE,lefttoright,generation,fn);
	if (doallsiblings>=2 && lefttoright)  Layout_TraverseTree(layout,Database_GetSiblingRtoL(person),2,3,Desk_TRUE,Desk_FALSE,lefttoright,generation,fn);
	if (doallsiblings>=2 && !lefttoright) Layout_TraverseTree(layout,Database_GetSiblingLtoR(person),2,3,Desk_TRUE,Desk_FALSE,lefttoright,generation,fn);
	if (dochild && Database_GetMarriage(person)) {
		if (lefttoright) {
			if (Database_GetMarriageRtoL(person)!=Database_GetPrincipalFromMarriage(Database_GetMarriage(person))) {
				Layout_TraverseTree(layout,Database_GetLeftChild(Database_GetMarriage(person)),2,2,Desk_TRUE,Desk_FALSE,lefttoright,generation+1,fn);
			}
		} else {
			if (Database_GetPrincipalFromMarriage(Database_GetMarriage(person))!=person) {
				Layout_TraverseTree(layout,Database_GetRightChild(Database_GetMarriage(person)),2,2,Desk_TRUE,Desk_FALSE,lefttoright,generation+1,fn);
			}
		}
	}
	fn(layout,person,generation,lefttoright,Desk_TRUE);
	if ((domarriage==Desk_TRUE || domarriage==2) && lefttoright)  Layout_TraverseTree(layout,Database_GetMarriageLtoR(person),Desk_TRUE,2,Desk_TRUE,Desk_TRUE,lefttoright,generation,fn);
	if ((domarriage==Desk_TRUE || domarriage==2) && !lefttoright) Layout_TraverseTree(layout,Database_GetMarriageRtoL(person),Desk_TRUE,2,Desk_TRUE,Desk_TRUE,lefttoright,generation,fn);
	if ((doallsiblings==1 || doallsiblings==2) &&lefttoright)  Layout_TraverseTree(layout,Database_GetSiblingLtoR(person),2,1,Desk_TRUE,Desk_FALSE,lefttoright,generation,fn);
	if ((doallsiblings==1 || doallsiblings==2) &&!lefttoright) Layout_TraverseTree(layout,Database_GetSiblingRtoL(person),2,1,Desk_TRUE,Desk_FALSE,lefttoright,generation,fn);
	if (doparents) {
		if (lefttoright && Database_GetMarriageRtoL(Database_GetMother(person))==Database_GetFather(person)) {
			Layout_TraverseTree(layout,Database_GetFather(person),2,2,Desk_FALSE,Desk_TRUE,lefttoright,generation-1,fn);
		} else {
			Layout_TraverseTree(layout,Database_GetMother(person),2,2,Desk_FALSE,Desk_TRUE,lefttoright,generation-1,fn);
		}
	}
*Traversing righttoleft is broken*
}*/

static void Layout_TraverseNormalTree(layout *layout,elementptr person,int generation,callfn fn)
{
	AJWLib_Assert(layout!=NULL);
	AJWLib_Assert(fn!=NULL);
	if (person==none) return;
	/* Check that we have not done this person already*/
	if (Layout_GetSelect(person)) return;
	Layout_Select(person);

	/* Start with left most sibling*/
	Layout_TraverseNormalTree(layout,Database_GetSiblingRtoL(person),generation,fn);
	/* Then any children*/
	Layout_TraverseNormalTree(layout,Database_GetLeftChild(Database_GetMarriage(person)),generation+1,fn);
	/* Plot the person*/
	fn(layout,person,generation);
    /* Do any spouses*/
	Layout_TraverseNormalTree(layout,Database_GetMarriageLtoR(person),generation,fn);
	Layout_TraverseNormalTree(layout,Database_GetMarriageRtoL(person),generation,fn);
	Layout_TraverseNormalTree(layout,Database_GetSiblingLtoR(person),generation,fn);
	Layout_TraverseNormalTree(layout,Database_GetFather(person),generation-1,fn);
}

/*static void Layout_TraverseAncestorTree(layout *layout,elementptr person,int generation,callfn fn)
{
	elementptr principal;
	AJWLib_Assert(layout!=NULL);
	if (person==none) return;
	principal=Database_GetPrincipalFromMarriage(Database_GetMarriage(person));
	Layout_TraverseAncestorTree(layout,Database_GetMother(person),generation-1,fn);
	if (person!=principal && principal!=none) Layout_TraverseAncestorTree(layout,Database_GetMother(principal),generation-1,fn);
	fn(layout,person,generation,Desk_FALSE,Desk_TRUE);
	if (person!=principal && principal!=none) fn(layout,principal,generation,Desk_FALSE,Desk_TRUE);
}

static void Layout_TraverseDescendentTree(layout *layout,elementptr person,int domarriage,int generation,callfn fn)
{
	AJWLib_Assert(layout!=NULL);
	if (person==none) return;
	if (domarriage>=2) Layout_TraverseDescendentTree(layout,Database_GetMarriageLtoR(person),3,generation,fn);
	if (Database_GetPrincipalFromMarriage(Database_GetMarriage(person))!=person && generation<numgenerations-1) Layout_TraverseDescendentTree(layout,Database_GetRightChild(Database_GetMarriage(person)),2,generation+1,fn);
	fn(layout,person,generation,Desk_FALSE,Desk_TRUE);
	if (domarriage==Desk_TRUE || domarriage==2) Layout_TraverseDescendentTree(layout,Database_GetMarriageRtoL(person),Desk_TRUE,generation,fn);
	if (generation>0) Layout_TraverseDescendentTree(layout,Database_GetSiblingRtoL(person),2,generation,fn);
}*/

void Layout_LayoutMarriages(layout *layout)
{
	int i;
	AJWLib_Assert(layout!=NULL);
	layout->nummarriages=0;
	for (i=0;i<layout->numpeople;i++) Layout_PlotMarriage(layout,layout->person[i].element,layout->person[i].x,layout->person[i].y);
}

void Layout_LayoutLines(layout *layout)
{
	int i;
	AJWLib_Assert(layout!=NULL);
	layout->numchildren=0;
	for (i=0;i<layout->numpeople;i++) Layout_PlotChildLine(layout,layout->person[i].element,layout->person[i].y);
}

void Layout_SelectDescendents(layout *layout,elementptr person)
{
/*	AJWLib_Assert(layout!=NULL);
	AJWLib_Assert(person!=none);
	numgenerations=INFINITY;
	selectmarriages=Desk_TRUE;
	Layout_TraverseDescendentTree(layout,person,2,0,Layout_Select);
*/}

void Layout_SelectAncestors(layout *layout,elementptr person)
{
/*	AJWLib_Assert(layout!=NULL);
	AJWLib_Assert(person!=none);
	numgenerations=INFINITY;
	selectmarriages=Desk_TRUE;
	Layout_TraverseAncestorTree(layout,person,0,Layout_Select);
*/}

void Layout_SelectSiblings(layout *layout,elementptr person)
{
/*	elementptr person2=person;
	AJWLib_Assert(layout!=NULL);
	AJWLib_Assert(person!=none);
	selectmarriages=Desk_FALSE;
	while ((person=Database_GetSiblingLtoR(person))!=none) Layout_Select(layout,person,0,Desk_FALSE,Desk_FALSE);
	do Layout_Select(layout,person2,0,Desk_FALSE,Desk_FALSE); while ((person2=Database_GetSiblingRtoL(person2))!=none);
*/}

void Layout_SelectSpouses(layout *layout,elementptr person)
{
/*	elementptr person2=person;
	AJWLib_Assert(layout!=NULL);
	AJWLib_Assert(person!=none);
	selectmarriages=Desk_TRUE;
	while ((person=Database_GetMarriageLtoR(person))!=none) Layout_Select(layout,person,0,Desk_FALSE,Desk_FALSE);
	do Layout_Select(layout,person2,0,Desk_FALSE,Desk_FALSE); while ((person2=Database_GetMarriageRtoL(person2))!=none);
*/}

layout *Layout_LayoutNormal(void)
{
	elementptr person=none;
	layout *layout;
	int index=0;

	AJWLib_Assert(spaces==NULL);
	layout=Desk_DeskMem_Malloc(sizeof(struct layout));
    layout->person=NULL;
    layout->marriage=NULL;
    layout->children=NULL;
	layout->numpeople=0;
	layout->nummarriages=0;
	layout->numchildren=0;
    Desk_Error2_Try {
		AJWLib_Flex_Alloc((flex_ptr)&(layout->person),1);
		AJWLib_Flex_Alloc((flex_ptr)&(layout->marriage),1);
		AJWLib_Flex_Alloc((flex_ptr)&(layout->children),1);
		AJWLib_Flex_Alloc((flex_ptr)&(spaces),sizeof(line));
		spaces[0].minx=0;
		spaces[0].maxx=0;
		mingeneration=0;
		maxgeneration=0;
		if (Config_Title()) {
			layout->title.x=0;
			layout->title.y=0;
		}
		/* Deselect everyone*/
		Layout_DeSelectAll();
		do {
			/* Get someone to start off with*/
			person=Database_GetLinked(&index);
			/* Get as far up the tree as we can*/
			while (Database_GetFather(person)!=none) person=Database_GetFather(person);
			/* Recursivly do the business*/
			Layout_TraverseNormalTree(layout,person,0,Layout_Plot);
		} while (index);
		/* Sort out marriages and lines*/
		Layout_LayoutMarriages(layout);
		Layout_PlotBodgedMarriages(layout);
		Layout_LayoutLines(layout);
		Layout_LayoutTitle(layout);
		/* Deselect everyone*/
		Layout_DeSelectAll();
		AJWLib_Flex_Free((flex_ptr)&spaces);
	} Desk_Error2_Catch {
		if (spaces) AJWLib_Flex_Free((flex_ptr)&spaces);
		if (layout->person) AJWLib_Flex_Free((flex_ptr)&(layout->person));
		if (layout->marriage) AJWLib_Flex_Free((flex_ptr)&(layout->marriage));
		if (layout->children) AJWLib_Flex_Free((flex_ptr)&(layout->children));
		free(layout);
		Desk_Error2_ReThrow();
	} Desk_Error2_EndCatch
	return layout;
}

layout *Layout_LayoutDescendents(elementptr person,int generations)
{
/*	layout *layout=NULL;
	AJWLib_Assert(person!=none);
	AJWLib_Assert(spaces==NULL);
	layout=Desk_DeskMem_Malloc(sizeof(struct layout));
	layout->person=NULL;
	layout->marriage=NULL;
	layout->children=NULL;
	layout->numpeople=0;
	layout->nummarriages=0;
	layout->numchildren=0;
	Desk_Error2_Try {
		AJWLib_Flex_Alloc((flex_ptr)&(layout->person),1);
		AJWLib_Flex_Alloc((flex_ptr)&(layout->marriage),1);
		AJWLib_Flex_Alloc((flex_ptr)&(layout->children),1);
		AJWLib_Flex_Alloc((flex_ptr)&(spaces),sizeof(line));
		spaces[0].minx=0;
		spaces[0].maxx=0;
		mingeneration=0;
		maxgeneration=0;
		numgenerations=generations;
		layout->title.x=INFINITY;
		layout->title.y=INFINITY;
		largenumber=0;
		firstplot=Desk_TRUE;
		Layout_TraverseDescendentTree(layout,person,2,0,Layout_Plot);
		Layout_LayoutMarriages(layout);
		Layout_LayoutLines(layout);
		AJWLib_Flex_Free((flex_ptr)&spaces);
	} Desk_Error2_Catch {
		if (spaces) AJWLib_Flex_Free((flex_ptr)&spaces);
		if (layout->person) AJWLib_Flex_Free((flex_ptr)&(layout->person));
		if (layout->marriage) AJWLib_Flex_Free((flex_ptr)&(layout->marriage));
		if (layout->children) AJWLib_Flex_Free((flex_ptr)&(layout->children));
		free(layout);
		Desk_Error2_ReThrow();
	} Desk_Error2_EndCatch
	return layout;
*/
return NULL;
}

layout *Layout_LayoutAncestors(elementptr person,int generations)
{
/*	layout *layout=NULL;
	AJWLib_Assert(person!=none);
	AJWLib_Assert(spaces==NULL);
	layout=Desk_DeskMem_Malloc(sizeof(struct layout));
	layout->person=NULL;
	layout->marriage=NULL;
	layout->children=NULL;
	layout->numpeople=0;
	layout->nummarriages=0;
	layout->numchildren=0;
	Desk_Error2_Try {
		AJWLib_Flex_Alloc((flex_ptr)&(layout->person),1);
		AJWLib_Flex_Alloc((flex_ptr)&(layout->marriage),1);
		AJWLib_Flex_Alloc((flex_ptr)&(layout->children),1);
		AJWLib_Flex_Alloc((flex_ptr)&(spaces),sizeof(line));
		spaces[0].minx=0;
		spaces[0].maxx=0;
		mingeneration=0;
		maxgeneration=0;
		numgenerations=generations; *?*
		layout->title.x=INFINITY;
		layout->title.y=INFINITY;
		largenumber=0;
		firstplot=Desk_TRUE;
		Layout_TraverseAncestorTree(layout,person,-generations,Layout_PlotAncestors);
		Layout_LayoutMarriages(layout);
		Layout_LayoutLines(layout);
		AJWLib_Flex_Free((flex_ptr)&spaces);
	} Desk_Error2_Catch {
		if (spaces) AJWLib_Flex_Free((flex_ptr)&spaces);
		if (layout->person) AJWLib_Flex_Free((flex_ptr)&(layout->person));
		if (layout->marriage) AJWLib_Flex_Free((flex_ptr)&(layout->marriage));
		if (layout->children) AJWLib_Flex_Free((flex_ptr)&(layout->children));
		free(layout);
		Desk_Error2_ReThrow();
	} Desk_Error2_EndCatch
	return layout;
*/
return NULL;
}


void Layout_SaveGEDCOM(layout *layout,FILE *file)
/*Save a GEDCOM layout to the given file ptr*/
{
	int i;
	
	AJWLib_Assert(layout!=NULL);
	AJWLib_Assert(file!=NULL);

	fprintf(file,"0 @L1@ _LAYOUT\n");
	for (i=0;i<layout->numpeople;i++) {
		fprintf(file,"1 _PERSON @%d@\n",layout->person[i].element);
		fprintf(file,"2 _X %d\n",layout->person[i].x);
		fprintf(file,"2 _Y %d\n",layout->person[i].y);
	}
	for (i=0;i<layout->nummarriages;i++) {
		fprintf(file,"1 _MARRIAGE @%d@\n",layout->marriage[i].element);
		fprintf(file,"2 _X %d\n",layout->marriage[i].x);
		fprintf(file,"2 _Y %d\n",layout->marriage[i].y);
	}
}

layout *Layout_GetGEDCOMLayout(void)
/* Return the GEDCOM layout used while loading, then reset it ready for the next load*/
{
	layout *returnvalue=gedcomlayout;
	gedcomlayout=NULL;
	return returnvalue;
}

void Layout_GEDCOMNewPerson(elementptr person)
/* Add a new person to the GEDCOM layout*/
{
	if (gedcomlayout==NULL) {
		gedcomlayout=Desk_DeskMem_Malloc(sizeof(struct layout));
		gedcomlayout->numpeople=0;
		gedcomlayout->nummarriages=0;
		gedcomlayout->numchildren=0;
		AJWLib_Flex_Alloc((flex_ptr)&(gedcomlayout->person),1);
		AJWLib_Flex_Alloc((flex_ptr)&(gedcomlayout->marriage),1);
		AJWLib_Flex_Alloc((flex_ptr)&(gedcomlayout->children),1);
	}
	AJWLib_Flex_Extend((flex_ptr)&(gedcomlayout->person),(gedcomlayout->numpeople+1)*sizeof(elementlayout));
	gedcomlayout->person[gedcomlayout->numpeople].element=person;
	gedcomlayout->person[gedcomlayout->numpeople].x=0;
	gedcomlayout->person[gedcomlayout->numpeople].y=0;
	gedcomlayout->numpeople++;
}

void Layout_GEDCOMNewPersonX(int pos)
/* Add the x coord to a new person to the GEDCOM layout*/
{
	AJWLib_Assert(gedcomlayout!=NULL);
	gedcomlayout->person[gedcomlayout->numpeople-1].x=pos;
}

void Layout_GEDCOMNewPersonY(int pos)
/* Add the y coord to a new person to the GEDCOM layout*/
{
	AJWLib_Assert(gedcomlayout!=NULL);
	gedcomlayout->person[gedcomlayout->numpeople-1].y=Layout_NearestGeneration(pos);
}

void Layout_GEDCOMNewMarriage(elementptr marriage)
/* Add a new marriage to the GEDCOM layout*/
{
	if (gedcomlayout==NULL) {
		gedcomlayout=Desk_DeskMem_Malloc(sizeof(struct layout));
		gedcomlayout->numpeople=0;
		gedcomlayout->nummarriages=0;
		gedcomlayout->numchildren=0;
		AJWLib_Flex_Alloc((flex_ptr)&(gedcomlayout->person),1);
		AJWLib_Flex_Alloc((flex_ptr)&(gedcomlayout->marriage),1);
		AJWLib_Flex_Alloc((flex_ptr)&(gedcomlayout->children),1);
	}
	AJWLib_Flex_Extend((flex_ptr)&(gedcomlayout->marriage),(gedcomlayout->nummarriages+1)*sizeof(elementlayout));
	gedcomlayout->marriage[gedcomlayout->nummarriages].element=marriage;
	gedcomlayout->marriage[gedcomlayout->nummarriages].x=0;
	gedcomlayout->marriage[gedcomlayout->nummarriages].x=0;
	gedcomlayout->nummarriages++;
}

void Layout_GEDCOMNewMarriageX(int pos)
/* Add the x coord to a new marriage to the GEDCOM layout*/
{
	AJWLib_Assert(gedcomlayout!=NULL);
	gedcomlayout->marriage[gedcomlayout->nummarriages-1].x=pos;
}

void Layout_GEDCOMNewMarriageY(int pos)
/* Add the y coord to a new marriage to the GEDCOM layout*/
{
	AJWLib_Assert(gedcomlayout!=NULL);
	gedcomlayout->marriage[gedcomlayout->nummarriages-1].y=Layout_NearestGeneration(pos);
}

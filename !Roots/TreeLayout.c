/*
	Roots - Tree related layout routines
	© Alex Waugh 1999

	$Id: TreeLayout.c,v 1.53 2000/11/21 20:04:26 AJW Exp $

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

typedef void (*callfn)(layout *layout,elementptr person,int);

static struct {
	int minx;
	int maxx;
} *spaces;

static int mingeneration,maxgeneration;


void TreeLayout_CheckForUnlink(layout *layout,elementptr i)
{
	elementptr marriage;
	int y;

	y=Layout_FindYCoord(layout,i);
	switch (Database_GetElementType(i)) {
		case element_PERSON:
			/*Get parents marriage*/
			if ((marriage=Database_GetParentsMarriage(i))!=none) {
				/*If person is above parents marriage then unlink them from the marriage and their siblings*/
				if (y>=Layout_FindYCoord(layout,marriage)) Database_UnlinkFromSiblingsAndParents(i,marriage);
			}
			if ((marriage=Database_GetMarriage(i))!=none) {
				/*If spouses are on different generations then remove the marriage*/
				if (Layout_FindYCoord(layout,Database_GetPrincipalFromMarriage(marriage))!=Layout_FindYCoord(layout,Database_GetSpouseFromMarriage(marriage))) {
					Database_RemoveElement(layout,marriage);
				} else if (Config_SeparateMarriages()) {
					/*If person is not on same generation as their marriage then remove the marriage*/
					if (y!=Layout_FindYCoord(layout,marriage)) {
						Database_RemoveElement(layout,marriage);
					}
				}
			}
			break;
		case element_MARRIAGE:
			if (Layout_FindYCoord(layout,Database_GetPrincipalFromMarriage(i))!=y || Layout_FindYCoord(layout,Database_GetSpouseFromMarriage(i))!=y) {
				Database_RemoveElement(layout,i);
			}
			break;
		default:
			break;
	}
}

void Layout_LayoutTitle(layout *layout)
/*Create the title transient, should be called after all other elements and transients have been placed*/
{
	Desk_wimp_rect bbox;
	flags flags;

	AJWLib_Assert(layout!=NULL);

	if (!Config_Title()) return;
	bbox=Layout_FindExtent(layout,Desk_FALSE);
	flags.editable=1;
	flags.moveable=0;
	flags.linkable=0;
	flags.snaptogrid=0;
	flags.selectable=0;
	Layout_AddTransient(layout,element_TITLE,bbox.min.x,bbox.max.y,bbox.max.x-bbox.min.x,Graphics_TitleHeight(),0,0,flags);
}

static void Layout_ExtendGeneration(int generation)
{
	int i;
	if (generation<mingeneration) {
		AJWLib_Flex_MidExtend((flex_ptr)&spaces,0,sizeof(*spaces)*(mingeneration-generation));
		for (i=0;i<mingeneration-generation;i++) {
			spaces[i].minx=0;
			spaces[i].maxx=0;
		}
		mingeneration=generation;
	}
	if (generation>maxgeneration) {
		AJWLib_Flex_Extend((flex_ptr)&spaces,sizeof(*spaces)*(generation-mingeneration+1));
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
			int leftpos,rightpos,marriagey;
			flags flags;

			flags.editable=0;
			flags.moveable=0;
			flags.linkable=0;
			flags.snaptogrid=0;
			flags.selectable=0;

			leftpos=rightpos=Layout_FindXCoord(layout,marriage)+Layout_FindWidth(layout,marriage)/2;
			marriagey=Layout_FindYCoord(layout,marriage)-Graphics_GapHeightBelow();
			do {
				/*Find leftmost and rightmost positions of siblings*/
				int x,y;
				x=Layout_FindXCoord(layout,person)+Layout_FindWidth(layout,person)/2;
				y=Layout_FindYCoord(layout,person)+Layout_FindHeight(layout,person);
				if (x<leftpos) leftpos=x;
				if (x>rightpos) rightpos=x;
				/*Create the vertical line*/
				Layout_AddTransient(layout,element_LINE,x,y+Graphics_GapHeightAbove(),0,marriagey-(y+Graphics_GapHeightAbove()),0,0,flags);
			} while ((person=Database_GetSiblingLtoR(person))!=none);
			/*Create the horizontal line*/
			Layout_AddTransient(layout,element_LINE,leftpos,marriagey,rightpos-leftpos,0,0,0,flags);
		}
	}
}

static void Layout_PlotPerson(layout *layout,elementptr person,int generation)
{
	elementptr marriage;
	flags flags;

	AJWLib_Assert(layout!=NULL);
	AJWLib_Assert(person!=none);
	marriage=Database_GetMarriage(person);
	if (marriage && Database_GetPrincipalFromMarriage(marriage)!=person) {
		spaces[generation-mingeneration].maxx+=Graphics_MarriageWidth();
		if (!Database_IsFirstMarriage(marriage)) spaces[generation-mingeneration].maxx+=Graphics_SecondMarriageGap();
	} else {
		spaces[generation-mingeneration].maxx+=Graphics_GapWidth();
	}
	flags.editable=1;
	flags.moveable=1;
	flags.linkable=1;
	flags.snaptogrid=1;
	flags.selectable=1;
	Layout_AddElement(layout,person,spaces[generation-mingeneration].maxx,Layout_NearestGeneration((generation)*-(Graphics_GapHeightAbove()+Graphics_GapHeightBelow()+Graphics_PersonHeight())),Graphics_PersonWidth(),Graphics_PersonHeight(),0,generation,flags);
	spaces[generation-mingeneration].maxx+=Graphics_PersonWidth();
}

static void Layout_PlotMarriage(layout *layout,elementptr marriage)
{
	elementptr leftperson,rightperson;
	flags flags;
	int leftx,rightx;

	AJWLib_Assert(layout!=NULL);
	AJWLib_Assert(marriage!=none);
	
	leftperson=Database_GetSpouseFromMarriage(marriage);
	leftx=Layout_FindXCoord(layout,leftperson);
	rightperson=Database_GetPrincipalFromMarriage(marriage);
	rightx=Layout_FindXCoord(layout,rightperson);
	if (leftx>rightx) {
		int temp1=leftx;
		elementptr temp2=leftperson;
		leftx=rightx;
		leftperson=rightperson;
		rightx=temp1;
		rightperson=temp2;
	}
	leftx+=Layout_FindWidth(layout,leftperson);
	flags.editable=1;
	flags.linkable=1;
	flags.snaptogrid=1;
	flags.selectable=1;
	if (Config_SeparateMarriages()) {
		flags.moveable=1;
		Layout_AddElement(layout,marriage,rightx-Graphics_MarriageWidth(),Layout_FindYCoord(layout,leftperson),Graphics_MarriageWidth(),Layout_FindHeight(layout,leftperson),0,0,flags);
	} else {
		flags.moveable=0;
		Layout_AddTransient(layout,marriage,leftx,Layout_FindYCoord(layout,leftperson),rightx-leftx,Layout_FindHeight(layout,leftperson),0,0,flags);
	}
}

void TreeLayout_AddMarriage(layout *layout,elementptr marriage)
{
	elementptr leftperson,rightperson;
	flags flags;
	int leftx,rightx;

	AJWLib_Assert(layout!=NULL);
	AJWLib_Assert(marriage!=none);

	if (!Config_SeparateMarriages()) return;
	leftperson=Database_GetSpouseFromMarriage(marriage);
	leftx=Layout_FindXCoord(layout,leftperson);
	rightperson=Database_GetPrincipalFromMarriage(marriage);
	rightx=Layout_FindXCoord(layout,rightperson);
	if (leftx>rightx) {
		int temp1=leftx;
		elementptr temp2=leftperson;
		leftx=rightx;
		leftperson=rightperson;
		rightx=temp1;
		rightperson=temp2;
	}
	flags.editable=1;
	flags.moveable=1;
	flags.linkable=1;
	flags.snaptogrid=1;
	flags.selectable=1;
	Layout_AddElement(layout,marriage,rightx-Graphics_MarriageWidth(),Layout_FindYCoord(layout,rightperson),Graphics_MarriageWidth(),Graphics_PersonHeight(),0,0,flags);
}

static int Layout_FindChildCoords(layout *layout,elementptr marriage)
{
	elementptr leftchild,rightchild;
	AJWLib_Assert(layout!=NULL);
	AJWLib_Assert(marriage!=none);
	leftchild=Database_GetLeftChild(marriage);
	rightchild=leftchild;
	while (Database_GetSiblingLtoR(rightchild)!=none) rightchild=Database_GetSiblingLtoR(rightchild);

	return (Layout_FindXCoord(layout,leftchild)+Layout_FindXCoord(layout,rightchild)+Graphics_PersonWidth())/2;
}

static void Layout_Plot(layout *layout,elementptr person,int generation)
{
	elementptr marriage;

	AJWLib_Assert(layout!=NULL);
	AJWLib_Assert(person!=none);

	/* Make sure we have set up structure for this generation*/
	Layout_ExtendGeneration(generation);

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

static void Layout_TraverseNormalTree(layout *layout,elementptr person,int generation,callfn fn)
{
	AJWLib_Assert(layout!=NULL);
	AJWLib_Assert(fn!=NULL);
	if (person==none) return;
	/* Check that we have not done this person already*/
	if (Database_GetFlag(person)) return;
	Database_SetFlag(person);

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

void Layout_LayoutLines(layout *layout,Desk_bool layoutseparatemarriages)
{
	int i=0;
	AJWLib_Assert(layout!=NULL);
	Layout_RemoveTransients(layout);
	if (!Config_SeparateMarriages() || layoutseparatemarriages) {
		do {
			int FIXME; /*FIXME: assumes everyone is on the layout*/
			elementptr marriage=Database_GetLinkedMarriages(&i);
	
			if (marriage) Layout_PlotMarriage(layout,marriage);
			
		} while (i);
	}
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
	layout=Layout_New();
    Desk_Error2_Try {
		AJWLib_Flex_Alloc((flex_ptr)&(spaces),sizeof(*spaces));
		spaces[0].minx=0;
		spaces[0].maxx=0;
		mingeneration=0;
		maxgeneration=0;
		/* Deselect everyone*/
		Database_UnsetAllFlags();
		do {
			/* Get someone to start off with*/
			person=Database_GetLinked(&index);
			/* Get as far up the tree as we can*/
			while (Database_GetFather(person)!=none) person=Database_GetFather(person);
			/* Recursivly do the business*/
			Layout_TraverseNormalTree(layout,person,0,Layout_Plot);
		} while (index);
		/* Sort out marriages and lines*/
		Layout_LayoutLines(layout,Desk_TRUE);
		Layout_LayoutTitle(layout);
		/* Deselect everyone*/
		Database_UnsetAllFlags();
		AJWLib_Flex_Free((flex_ptr)&spaces);
	} Desk_Error2_Catch {
		if (spaces) AJWLib_Flex_Free((flex_ptr)&spaces);
		Layout_Free(layout);
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


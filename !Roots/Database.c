/*
	Roots - Database
	© Alex Waugh 1999

	$Id: Database.c,v 1.60 2004/01/05 22:42:51 AJW Exp $

*/


#include "Desk.Window.h"
#include "Desk.Error2.h"
#include "Desk.Event.h"
#include "Desk.Template.h"
#include "Desk.File.h"
#include "Desk.Menu.h"
#include "Desk.Msgs.h"
#include "Desk.DeskMem.h"
#include "Desk.Filing.h"
#include "Desk.Str.h"

#include "AJWLib.Assert.h"
#include "AJWLib.Window.h"
#include "AJWLib.Error2.h"
#include "AJWLib.Menu.h"
#include "AJWLib.Msgs.h"
#include "AJWLib.Menu.h"
#include "AJWLib.Flex.h"
#include "AJWLib.File.h"

#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include "Database.h"
#include "Modules.h"
#include "File.h"
#include "Layout.h"
#include "Config.h"
#include "Graphics.h"
#include "Shareware.h"


#define editpersonicon_SURNAME 1
#define editpersonicon_FORENAME 7
#define editpersonicon_MIDDLENAMES 8
#define editpersonicon_SEX 9
#define editpersonicon_OK 3
#define editpersonicon_CANCEL 2
#define editpersonicon_SEXMENU 10
#define editpersonicon_USERBASE 11

#define edittitleicon_TEXT 0
#define edittitleicon_CANCEL 1
#define edittitleicon_OK 2

#define editmarriageicon_PRINCIPAL 2
#define editmarriageicon_SPOUSE 3
#define editmarriageicon_OK 1
#define editmarriageicon_CANCEL 0
#define editmarriageicon_USERBASE 4

#define sexmenu_M 0
#define sexmenu_F 1
#define sexmenu_U 2

typedef struct persondata {
	char *surname;
	char *forename;
	char *middlenames;
	sextype sex;
    char *user[NUMBERPERSONUSERFIELDS];
} persondata;

typedef struct person {
	elementptr parentsmarriage;
	elementptr siblingsrtol;
	elementptr siblingsltor;
	elementptr marriage;
	persondata data;
} person;

typedef struct freeelement {
	elementptr next;
} freeelement;

typedef struct marriagedata {
	char *user[NUMBERMARRIAGEUSERFIELDS];
} marriagedata;

typedef struct marriage {
	elementptr principal;
	elementptr spouse;
	elementptr leftchild;
	elementptr rightchild;
	elementptr next;
	elementptr previous;
	marriagedata data;
} marriage;

typedef struct file {
	int numberofelements;
	int newpersonnumber;
	char *filetitle;
	elementptr freeelement;
} file;

typedef union element {
	person person;
	freeelement freeelement;
	marriage marriage;
	file file;
} element;

typedef struct databaseelement {
	elementtype type;
	element element;
	Desk_bool selected;
} databaseelement;

static char personuser[NUMBERPERSONUSERFIELDS][FIELDSIZE];
static char personGEDCOM[NUMBERPERSONUSERFIELDS][FIELDSIZE];
static char marriageuser[NUMBERMARRIAGEUSERFIELDS][FIELDSIZE];
static char marriageGEDCOM[NUMBERMARRIAGEUSERFIELDS][FIELDSIZE];
static databaseelement *database=NULL;
static elementptr editingperson=none,editingmarriage=none;
static Desk_window_handle editpersonwin,editmarriagewin,edittitlewin;
static Desk_menu_ptr sexmenu;

elementtype Database_GetElementType(elementptr element)
{
	AJWLib_Assert(database!=NULL);
	
	if (element<=0) return (elementtype)element;
	AJWLib_AssertWarning(element<database[0].element.file.numberofelements);
	if (element>=database[0].element.file.numberofelements) return element_NONE;
	return database[element].type;
}

elementptr Database_GetLinked(int *index)
/* Get the next person in the database*/
{
	AJWLib_Assert(database!=NULL);
	
	while ((++(*index))<database[0].element.file.numberofelements) {
		if (database[*index].type==element_PERSON) {
			return *index;
		}
	}
	*index=0;
	return none;
}

elementptr Database_GetLinkedMarriages(int *index)
/* Get the next marriage in the database*/
{
	AJWLib_Assert(database!=NULL);
	
	while ((++(*index))<database[0].element.file.numberofelements) {
		if (database[*index].type==element_MARRIAGE) {
			return *index;
		}
	}
	*index=0;
	return none;
}

Desk_bool Database_Married(elementptr person1,elementptr person2)
/*Return TRUE if the two people are married to each other*/
{
	elementptr marriage;

	marriage=Database_GetMarriage(person1);
	while (marriage) {
		if (database[marriage].element.marriage.spouse==person1 && database[marriage].element.marriage.principal==person2) return Desk_TRUE;
		if (database[marriage].element.marriage.spouse==person2 && database[marriage].element.marriage.principal==person1) return Desk_TRUE;
		marriage=database[marriage].element.marriage.next;
	}
	marriage=Database_GetMarriage(person1);
	while (marriage) {
		if (database[marriage].element.marriage.spouse==person1 && database[marriage].element.marriage.principal==person2) return Desk_TRUE;
		if (database[marriage].element.marriage.spouse==person2 && database[marriage].element.marriage.principal==person1) return Desk_TRUE;
		marriage=database[marriage].element.marriage.previous;
	}
	return Desk_FALSE;
}

Desk_bool Database_ElementValid(elementptr person)
/*Check to see if a given elementptr is valid*/
{
	if (person>=database[0].element.file.numberofelements) return Desk_FALSE;
	if (person<0) return Desk_TRUE;
	if (database[person].type==element_PERSON || database[person].type==element_MARRIAGE) return Desk_TRUE;
	return Desk_FALSE;
}

#define Database_UpdateField(field,text) \
{\
	char *newtext;\
	newtext = Desk_DeskMem_Malloc(strlen(text)+1);\
	if (field != NULL) free(field);\
	field = newtext;\
	strcpy(field,text);\
}

void Database_SetTitle(char *title)
{
	Database_UpdateField(database[0].element.file.filetitle,title);
}

void Database_SetNextNewPerson(int personnumber)
{
	database[0].element.file.newpersonnumber=personnumber;
}

void Database_SetForename(elementptr person,char *name)
{
	Database_UpdateField(database[person].element.person.data.forename,name);
}

void Database_SetMiddleNames(elementptr person,char *name)
{
	Database_UpdateField(database[person].element.person.data.middlenames,name);
}

void Database_SetSurname(elementptr person,char *name)
{
	Database_UpdateField(database[person].element.person.data.surname,name);
}

char *Database_GetForename(elementptr person)
{
	return database[person].element.person.data.forename;
}

char *Database_GetMiddleNames(elementptr person)
{
	return database[person].element.person.data.middlenames;
}

char *Database_GetSurname(elementptr person)
{
	return database[person].element.person.data.surname;
}

void Database_SetSex(elementptr person,sextype sexchar)
{
	database[person].element.person.data.sex=sexchar;
}

void Database_SetPersonUser(int num,elementptr person,char *str)
{
	AJWLib_Assert(num<NUMBERPERSONUSERFIELDS);
	Database_UpdateField(database[person].element.person.data.user[num],str);
}

void Database_SetMarriageUser(int num,elementptr marriage,char *str)
{
	AJWLib_Assert(num<NUMBERMARRIAGEUSERFIELDS);
	Database_UpdateField(database[marriage].element.marriage.data.user[num],str);
}

char *Database_GetField(elementptr element,char *fieldname)
{
	static char sexstr[]="U";
	int i;

	AJWLib_Assert(database!=NULL);
	if (element<=none) return "";
	if (element>=database[0].element.file.numberofelements) return "";
	switch (database[element].type) {
		case element_PERSON:
			sexstr[0]=database[element].element.person.data.sex;
			if (Desk_stricmp(fieldname,"forename")==0)    return database[element].element.person.data.forename;
			if (Desk_stricmp(fieldname,"surname")==0)     return database[element].element.person.data.surname;
			if (Desk_stricmp(fieldname,"middlenames")==0) return database[element].element.person.data.middlenames;
			if (Desk_stricmp(fieldname,"sex")==0)         return sexstr;
			for (i=0;i<NUMBERPERSONUSERFIELDS;i++) {
				if (Desk_stricmp(fieldname,personuser[i])==0) return database[element].element.person.data.user[i];
			}
			break;
		case element_MARRIAGE:
			for (i=0;i<NUMBERMARRIAGEUSERFIELDS;i++) {
				if (Desk_stricmp(fieldname,marriageuser[i])==0) return database[element].element.marriage.data.user[i];
			}
			break;
	}
	return "";
}

void Database_SetMarriage(elementptr person,elementptr marriage)
/*Set the person to point to the marriage, or link the marriage into the chain if it is a second marriage*/
{
	elementptr existingmarriage;

	AJWLib_Assert(person!=none);
	AJWLib_Assert(marriage!=none);
	if ((existingmarriage=database[person].element.person.marriage)!=none) {
		while (database[existingmarriage].element.marriage.next) existingmarriage=database[existingmarriage].element.marriage.next;
		database[existingmarriage].element.marriage.next=marriage;
		database[marriage].element.marriage.previous=existingmarriage;
	} else {
		database[person].element.person.marriage=marriage;
	}
}

void Database_SetParentsMarriage(elementptr person,elementptr marriage)
{
	database[person].element.person.parentsmarriage=marriage;
}

void Database_SetPrincipal(elementptr marriage,elementptr person)
{
	database[marriage].element.marriage.principal=person;
}

void Database_SetSpouse(elementptr marriage,elementptr person)
{
	database[marriage].element.marriage.spouse=person;
}

#define Database_CheckConsistencyPerson(person,i,typem) \
{\
	if (person==i) {\
		person=none;\
		Desk_Msgs_Report(1,"Error.Dizzy:%s",Database_GetFullName(i));\
	}\
	if (person<0 || person>=database[0].element.file.numberofelements) {\
		person=none;\
		Desk_Msgs_Report(1,"Error.Elvis:%s",Database_GetFullName(i));\
	}\
	if (person) if (database[person].type!=typem) {\
		person=none;\
		Desk_Msgs_Report(1,"Error.Alien:%s",Database_GetFullName(i));\
	}\
}

#define Database_CheckConsistencyMarriage(person,i,typem) \
{\
	if (person==i) {\
		person=none;\
		Desk_Msgs_Report(1,"Error.DizzyM:");\
	}\
	if (person<0 || person>=database[0].element.file.numberofelements) {\
		person=none;\
		Desk_Msgs_Report(1,"Error.ElvisM:");\
	}\
	if (person) if (database[person].type!=typem) {\
		person=none;\
		Desk_Msgs_Report(1,"Error.AlienM:");\
	}\
}

void Database_CheckConsistency(void)
/* Check for invalid links between people*/
{
	int i;
	
	AJWLib_Assert(database!=NULL);
	for (i=1;i<database[0].element.file.numberofelements;i++) {
		switch (database[i].type) {
			case element_PERSON:
				Database_CheckConsistencyPerson(database[i].element.person.marriage,i,element_MARRIAGE);
				Database_CheckConsistencyPerson(database[i].element.person.parentsmarriage,i,element_MARRIAGE);
				Database_CheckConsistencyPerson(database[i].element.person.siblingsltor,i,element_PERSON);
				Database_CheckConsistencyPerson(database[i].element.person.siblingsltor,i,element_PERSON);
				break;
			case element_MARRIAGE:
				Database_CheckConsistencyMarriage(database[i].element.marriage.next,i,element_MARRIAGE);
				Database_CheckConsistencyMarriage(database[i].element.marriage.previous,i,element_MARRIAGE);
				Database_CheckConsistencyMarriage(database[i].element.marriage.principal,i,element_PERSON);
				Database_CheckConsistencyMarriage(database[i].element.marriage.spouse,i,element_PERSON);
				Database_CheckConsistencyMarriage(database[i].element.marriage.leftchild,i,element_PERSON);
				Database_CheckConsistencyMarriage(database[i].element.marriage.rightchild,i,element_PERSON);
				break;
			case element_FREE:
				if (database[i].element.freeelement.next==i) database[i].element.freeelement.next=none;
				if (database[i].element.freeelement.next<0 || database[i].element.freeelement.next>=database[0].element.file.numberofelements) database[i].element.freeelement.next=none;
				if (database[i].element.freeelement.next) if (database[database[i].element.freeelement.next].type!=element_FREE) database[i].element.freeelement.next=none;
				break;
			case element_FILE:
				break;
			default:
				AJWLib_Assert(0);
		}
	}
}

void Database_LinkAllChildren(void)
/* Link all children to their siblings and parents marriages*/
{
	int i;
	elementptr marriage,rightchild;

	AJWLib_Assert(database!=NULL);
	for (i=1;i<database[0].element.file.numberofelements;i++) {
		if (database[i].type==element_PERSON) {
			if ((marriage=database[i].element.person.parentsmarriage)!=none) {
				/* Set the marriage to point to the left and right most siblings*/
				if (database[marriage].element.marriage.leftchild==none) database[marriage].element.marriage.leftchild=i;
				rightchild=database[marriage].element.marriage.rightchild;
				database[marriage].element.marriage.rightchild=i;
				/* Add new child to right hand end of siblings chain*/
				if (rightchild!=none) database[rightchild].element.person.siblingsltor=i;
				database[i].element.person.siblingsrtol=rightchild;
				database[i].element.person.siblingsltor=none;
			}
		}
	}
}

void Database_LinkAllMarriages(void)
/* Check all marriage links*/
{
	int i;
	elementptr marriage;

	AJWLib_Assert(database!=NULL);
	for (i=1;i<database[0].element.file.numberofelements;i++) {
		if (database[i].type==element_PERSON) {
			if ((marriage=database[i].element.person.marriage)!=none) {
				/* Make sure one (and only one) of the pricipal or spouse point to this person*/
				if (database[marriage].element.marriage.principal==i) {
					if (database[marriage].element.marriage.spouse==i) database[marriage].element.marriage.spouse=none; /*Sort it out on next pass*/
				} else if (database[marriage].element.marriage.spouse==i) {
					if (database[marriage].element.marriage.principal==i) database[marriage].element.marriage.principal=none; /*Sort it out on next pass*/
				} else {
					/* Neither points to this person, so if either are null then set them to this person*/
					if (database[marriage].element.marriage.principal==none) {
						database[marriage].element.marriage.principal=i;
					} else if (database[marriage].element.marriage.spouse==none) {
						database[marriage].element.marriage.spouse=i;
					} else {
						/* Remove link to the marriage, as it must be wrong*/
						database[i].element.person.marriage=none;
					}
				}
			}
		}
	}
	/* Now go though each marriage and sort out all pointers to null people*/
	for (i=1;i<database[0].element.file.numberofelements;i++) {
		if (database[i].type==element_MARRIAGE) {
			if (database[i].element.marriage.principal==none) {
				elementptr newperson;

				newperson=Database_Add();
				database[i].element.marriage.principal=newperson;
				database[newperson].element.person.marriage=i;
			} else {
				/* Check that the principal does point to a marriage (needn't be this one)*/
				elementptr person;

				person=database[i].element.marriage.principal;
				if (database[person].element.person.marriage==none) database[person].element.person.marriage=i;
			}
			if (database[i].element.marriage.spouse==none) {
				elementptr newperson;

				newperson=Database_Add();
				database[i].element.marriage.spouse=newperson;
				database[newperson].element.person.marriage=i;
			} else {
				/* Check that the spouse does point to a marriage (needn't be this one)*/
				elementptr person;

				person=database[i].element.marriage.spouse;
				if (database[person].element.person.marriage==none) database[person].element.person.marriage=i;
			}
		}
	}
}

Desk_bool Database_LinkValid(layout *layout,elementptr start,elementptr end)
/*Check if a link from start to end would be valid*/
{
	if (Database_GetElementType(start)!=element_PERSON) return Desk_FALSE;
	switch (Database_GetElementType(end)) {
		case element_PERSON:
			/*Check that the dragged person is not the same as the destination person*/
			if (start==end) return Desk_FALSE;
			/*Check that both people are in the same generation*/
			if (Layout_FindYCoord(layout,start)!=Layout_FindYCoord(layout,end)) return Desk_FALSE; /*Use grid coordinates?*/
			return Desk_TRUE;
			break;

		case element_MARRIAGE:
			/*Check that the person does not have parents already*/
			if (Database_GetMother(start)) return Desk_FALSE;
			/*Check that the person is in the right generation (anything below the marriage is now ok*/
			if (Layout_FindYCoord(layout,start)>Layout_FindYCoord(layout,end)) return Desk_FALSE;
			return Desk_TRUE;
			break;
	}
	return Desk_FALSE;
}

elementptr Database_Link(layout *layout,elementptr start,elementptr end)
/*Link two elements together if possible*/
{
	if (!Database_LinkValid(layout,start,end)) return none;
	switch (Database_GetElementType(end)) {
		case element_PERSON:
			return Database_Marry(start,end);
			break;

		case element_MARRIAGE:
			Database_AddChild(end,start);
			break;
	}
	return none;
}

static void Database_FreeElement(elementptr element)
{
	int i;

	AJWLib_Assert(database!=NULL);
	if (database[element].type == element_PERSON) {
		if (database[element].element.person.data.forename) free(database[element].element.person.data.forename);
		if (database[element].element.person.data.middlenames) free(database[element].element.person.data.middlenames);
		if (database[element].element.person.data.surname) free(database[element].element.person.data.surname);
		for (i=0;i<NUMBERPERSONUSERFIELDS;i++) if (database[element].element.person.data.user[i]) free(database[element].element.person.data.user[i]);
	} else if (database[element].type == element_MARRIAGE) {
		for (i=0;i<NUMBERMARRIAGEUSERFIELDS;i++) if (database[element].element.marriage.data.user[i]) free(database[element].element.marriage.data.user[i]);
	}
	database[element].type=element_FREE;
	database[element].selected=Desk_FALSE;
	database[element].element.freeelement.next=database[0].element.file.freeelement;
	database[0].element.file.freeelement=element;
}

void Database_SetFlag(elementptr person)
{
	AJWLib_Assert(database!=NULL);
	AJWLib_Assert(person!=none);
	database[person].selected=Desk_TRUE;
}

void Database_UnsetFlag(elementptr person)
{
	AJWLib_Assert(database!=NULL);
	AJWLib_Assert(person!=none);
	database[person].selected=Desk_FALSE;
}

void Database_UnsetAllFlags(void)
{
	int i;
	AJWLib_Assert(database!=NULL);
	for (i=0;i<database[0].element.file.numberofelements;i++) {
		if (database[i].type==element_PERSON || database[i].type==element_MARRIAGE) {
			database[i].selected=Desk_FALSE;
		}
	}
}

elementtype Database_AnyoneFlagged(void)
{
	int i;
	elementtype selected=element_NONE;
	AJWLib_Assert(database!=NULL);
	for (i=1;i<database[0].element.file.numberofelements;i++) {
		if (database[i].selected) {
			if (database[i].type==element_PERSON || database[i].type==element_MARRIAGE) {
				switch (selected) {
					case element_NONE:
						selected=database[i].type;
						break;
					case element_PERSON:
					case element_MARRIAGE:
						selected=element_SELECTION;
						return selected;
					default:
						break;
				}
			}
		}
	}
	return selected;
}

Desk_bool Database_GetFlag(elementptr person)
{
	AJWLib_Assert(database!=NULL);
	AJWLib_Assert(person!=none);
	return database[person].selected;
}

void Database_RemoveElement(layout *layout,elementptr element)
{
	if (element==none) return;
	if (database[element].type==element_MARRIAGE) {
		elementptr child;
		/*Remove marriage from chain*/
		if (database[element].element.marriage.previous==none) {
			database[database[element].element.marriage.principal].element.person.marriage=database[element].element.marriage.next;
			if (database[element].element.marriage.next) {
				database[database[element].element.marriage.next].element.marriage.previous=none;
			}
		} else {
			database[database[element].element.marriage.previous].element.marriage.next=database[element].element.marriage.next;
			if (database[element].element.marriage.next) {
				database[database[element].element.marriage.next].element.marriage.previous=database[element].element.marriage.previous;
			}
		}
		/*Remove marriage from spouse*/
		database[database[element].element.marriage.spouse].element.person.marriage=none;
		/*Unlink any children from this marriage and their siblings*/
		child=database[element].element.marriage.leftchild;
		while (child) {
			int temp;
			database[child].element.person.parentsmarriage=none;
			database[child].element.person.siblingsrtol=none;
			temp=database[child].element.person.siblingsltor;
			database[child].element.person.siblingsltor=none;
			child=temp;
		}
		Layout_RemoveElement(layout,element);
		Database_FreeElement(element);
		Modules_ChangedStructure();
	} else if (database[element].type==element_PERSON) {
		Database_UnlinkFromSiblingsAndParents(element,database[element].element.person.parentsmarriage);
		Layout_RemoveElement(layout,database[element].element.person.marriage);
		Database_RemoveElement(layout,database[element].element.person.marriage);
		Layout_RemoveElement(layout,element);
		Database_FreeElement(element);
		Modules_ChangedStructure();
	}
}

void Database_UnlinkFromSiblingsAndParents(elementptr person,elementptr marriage)
{
	if (person==none) return;
	if (marriage==none) return;
	/*Remove from siblings chain*/
	if (database[person].element.person.siblingsltor) database[database[person].element.person.siblingsltor].element.person.siblingsrtol=database[person].element.person.siblingsrtol;
	if (database[person].element.person.siblingsrtol) database[database[person].element.person.siblingsrtol].element.person.siblingsltor=database[person].element.person.siblingsltor;
	/*Remove from parents marriage*/
	if (database[marriage].element.marriage.leftchild==person) database[marriage].element.marriage.leftchild=database[person].element.person.siblingsltor;
	if (database[marriage].element.marriage.rightchild==person) database[marriage].element.marriage.rightchild=database[person].element.person.siblingsrtol;
	/*Remove parents*/
	database[person].element.person.parentsmarriage=none;
}

elementptr Database_GetParentsMarriage(elementptr person)
{
	AJWLib_Assert(database!=NULL);
	if (person==none) return none;
	return database[person].element.person.parentsmarriage;
}

elementptr Database_GetMother(elementptr person)
{
	AJWLib_Assert(database!=NULL);
	AJWLib_AssertWarning(person>=0);
	AJWLib_AssertWarning(person<database[0].element.file.numberofelements);
	if (person<=none || person>=database[0].element.file.numberofelements) return none;
	if (database[person].element.person.parentsmarriage==none) return none;
	return database[database[person].element.person.parentsmarriage].element.marriage.spouse;
}

elementptr Database_GetFather(elementptr person)
{
	AJWLib_Assert(database!=NULL);
	if (person==none) return none;
	if (database[person].element.person.parentsmarriage==none) return none;
	return database[database[person].element.person.parentsmarriage].element.marriage.principal;
}

elementptr Database_GetSiblingRtoL(elementptr person)
{
	AJWLib_Assert(database!=NULL);
	if (person==none) return none;
	return database[person].element.person.siblingsrtol;
}

elementptr Database_GetSiblingLtoR(elementptr person)
{
	AJWLib_Assert(database!=NULL);
	if (person==none) return none;
	return database[person].element.person.siblingsltor;
}

elementptr Database_GetLeftChild(elementptr marriage)
{
	AJWLib_Assert(database!=NULL);
	if (marriage==none) return none;
	return database[marriage].element.marriage.leftchild;
}

elementptr Database_GetRightChild(elementptr marriage)
{
	AJWLib_Assert(database!=NULL);
	if (marriage==none) return none;
	return database[marriage].element.marriage.rightchild;
}

elementptr Database_GetSpouseFromMarriage(elementptr marriage)
{
	AJWLib_Assert(database!=NULL);
	if (marriage==none) return none;
	return database[marriage].element.marriage.spouse;
}

elementptr Database_GetPrincipalFromMarriage(elementptr marriage)
{
	AJWLib_Assert(database!=NULL);
	if (marriage==none) return none;
	return database[marriage].element.marriage.principal;
}

elementptr Database_GetMarriage(elementptr person)
{
	AJWLib_Assert(database!=NULL);
	if (person==none) return none;
	return database[person].element.person.marriage;
}

Desk_bool Database_IsFirstMarriage(elementptr marriage)
{
	AJWLib_Assert(database!=NULL);
	if (marriage==none) return Desk_FALSE;
	if (database[marriage].element.marriage.previous==none) return Desk_TRUE;
	return Desk_FALSE;
}

elementptr Database_GetMarriageLtoR(elementptr person)
{
	elementptr marriage;
	AJWLib_Assert(database!=NULL);
	if (person==none) return none;
	marriage=Database_GetMarriage(person);
	if (marriage==none) return none;
	if (person!=database[marriage].element.marriage.principal) marriage=database[marriage].element.marriage.next;
	if (marriage==none) return none;
	return database[marriage].element.marriage.spouse;
}

elementptr Database_GetMarriageRtoL(elementptr person)
{
	elementptr marriage,marriage2;
	AJWLib_Assert(database!=NULL);
	if (person==none) return none;
	marriage=Database_GetMarriage(person);
	if (marriage==none) return none;
	if (person==database[marriage].element.marriage.principal) return none;
	marriage2=database[marriage].element.marriage.previous;
	if (marriage2==none) return database[marriage].element.marriage.principal;
	return database[marriage2].element.marriage.spouse;
}

char *Database_GetName(elementptr person)
{
	static char result[256];
	AJWLib_Assert(database!=NULL);
	strcpy(result,"");
	if (strlen(database[person].element.person.data.forename)) {
		strcat(result,database[person].element.person.data.forename);
		strcat(result," ");
	}
	if (strlen(database[person].element.person.data.surname)) {
		strcat(result,database[person].element.person.data.surname);
	}
	return result;
}

sextype Database_GetSex(elementptr person)
{
	AJWLib_Assert(database!=NULL);
	AJWLib_Assert(person!=none);
	return database[person].element.person.data.sex;
}

char *Database_GetFullName(elementptr person)
{
	static char result[256];
	AJWLib_Assert(database!=NULL);
	AJWLib_Assert(person!=none);
	strcpy(result,"");
	if (strlen(database[person].element.person.data.forename)) {
		strcat(result,database[person].element.person.data.forename);
		strcat(result," ");
	}
	if (strlen(database[person].element.person.data.middlenames)) {
		strcat(result,database[person].element.person.data.middlenames);
		strcat(result," ");
	}
	if (strlen(database[person].element.person.data.surname)) {
		strcat(result,database[person].element.person.data.surname);
	}
	return result;
}

char *Database_GetInitialedMiddleName(elementptr person)
{
	static char result[256];
	AJWLib_Assert(database!=NULL);
	AJWLib_Assert(person!=none);
	strcpy(result,"");
	if (database[person].element.person.data.forename[0]!='\0') {
		strcat(result,database[person].element.person.data.forename);
		strcat(result," ");
	}
	if (database[person].element.person.data.middlenames[0]!='\0') {
		int len=strlen(result);
		result[len]=database[person].element.person.data.middlenames[0];
		result[len+1]=' ';
		result[len+2]='\0';
	}
	if (database[person].element.person.data.surname[0]!='\0') {
		strcat(result,database[person].element.person.data.surname);
	}
	return result;
}

char *Database_GetInitialedName(elementptr person)
{
	static char result[256];
	AJWLib_Assert(database!=NULL);
	AJWLib_Assert(person!=none);
	strcpy(result,"");
	if (database[person].element.person.data.forename[0]!='\0') {
		result[0]=database[person].element.person.data.forename[0];
		result[1]=' ';
		result[2]='\0';
	}
	if (database[person].element.person.data.middlenames[0]!='\0') {
		int len=strlen(result);
		result[len]=database[person].element.person.data.middlenames[0];
		result[len+1]=' ';
		result[len+2]='\0';
	}
	if (database[person].element.person.data.surname[0]!='\0') {
		strcat(result,database[person].element.person.data.surname);
	}
	return result;
}

char *Database_GetPersonUserField(elementptr person,int num)
{
	AJWLib_Assert(database!=NULL);
	AJWLib_Assert(person!=none);
	return database[person].element.person.data.user[num];
}

char *Database_GetMarriageUserField(elementptr marriage,int num)
{
	AJWLib_Assert(database!=NULL);
	AJWLib_Assert(marriage!=none);
	return database[marriage].element.marriage.data.user[num];
}

static elementptr Database_GetFreeElement(void)
{
	elementptr newelement=none;
	AJWLib_Assert(database!=NULL);
	if ((newelement=database[0].element.file.freeelement)==none) {
		if (database[0].element.file.numberofelements>UNREGISTEREDMAXPEOPLE) Shareware_CheckRegistered();
		AJWLib_Flex_Extend((flex_ptr)&database,sizeof(databaseelement)*(database[0].element.file.numberofelements+1));
		newelement=database[0].element.file.numberofelements++;
	} else {
		database[0].element.file.freeelement=database[newelement].element.freeelement.next;
	}
	return newelement;
}

elementptr Database_Marry(elementptr linked,elementptr unlinked)
{
	elementptr marriage,marriage2,marriage3;
	int i;
	
	AJWLib_Assert(database!=NULL);
	/*Check that these two people are not already married together*/
	if ((marriage=Database_GetMarriage(linked))==none) marriage=Database_GetMarriage(unlinked);
	marriage2=marriage;
	while (marriage2) {
		if ((database[marriage2].element.marriage.spouse==linked && database[marriage2].element.marriage.principal==unlinked) || (database[marriage2].element.marriage.spouse==unlinked && database[marriage2].element.marriage.principal==linked)) {
			AJWLib_Error2_HandleMsgs("Error.Marrd:");
		}
		marriage2=database[marriage2].element.marriage.previous;
	}
	marriage2=marriage;
	while (marriage2) {
		if ((database[marriage2].element.marriage.spouse==linked && database[marriage2].element.marriage.principal==unlinked) || (database[marriage2].element.marriage.spouse==unlinked && database[marriage2].element.marriage.principal==linked)) {
			AJWLib_Error2_HandleMsgs("Error.Marrd:");
		}
		marriage2=database[marriage2].element.marriage.next;
	}

	marriage=Database_GetFreeElement(); /*An error is ok, as we haven't altered the structure yet*/
	for (i=0;i<NUMBERMARRIAGEUSERFIELDS;i++) {
		database[marriage].element.marriage.data.user[i]=Desk_DeskMem_Malloc(1);
		database[marriage].element.marriage.data.user[i][0]='\0';
	}
	database[marriage].type=element_MARRIAGE;
	database[marriage].selected=Desk_FALSE;
	/*Link new marriage into marriage chain*/
	if ((marriage2=Database_GetMarriage(linked))==none) {
		if ((marriage3=Database_GetMarriage(unlinked))==none) {
			/*Neither people are married already*/
			database[marriage].element.marriage.previous=none;
			database[marriage].element.marriage.next=none;
		} else {
			/*unlinked is married already*/
			database[marriage].element.marriage.next=marriage3;
			database[marriage].element.marriage.previous=database[marriage3].element.marriage.previous;
			database[marriage3].element.marriage.previous=marriage;
			if (database[marriage].element.marriage.previous) database[database[marriage].element.marriage.previous].element.marriage.next=marriage;
		}
	} else {
		if ((marriage3=Database_GetMarriage(unlinked))==none) {
			/*linked is married already*/
			database[marriage].element.marriage.next=marriage2;
			database[marriage].element.marriage.previous=database[marriage2].element.marriage.previous;
			database[marriage2].element.marriage.previous=marriage;
			if (database[marriage].element.marriage.previous) database[database[marriage].element.marriage.previous].element.marriage.next=marriage;
		} else {
			/*Both are married already*/
			while (database[marriage2].element.marriage.next!=none) marriage2=database[marriage2].element.marriage.next;
			while (database[marriage3].element.marriage.next!=none) marriage3=database[marriage3].element.marriage.next;
			if (marriage2==marriage3) {
				/*Both people are already in the marriage chain (but not married to each other)*/
				/*Add the new marriage to the end of the chain*/
				database[marriage2].element.marriage.next=marriage;
				database[marriage].element.marriage.previous=marriage2;
				database[marriage].element.marriage.next=none;
			} else {
				/*Join the two marriage chains together via the new marriage*/
				while (database[marriage3].element.marriage.previous!=none) marriage3=database[marriage3].element.marriage.previous;
				database[marriage2].element.marriage.next=marriage;
				database[marriage].element.marriage.previous=marriage2;
				database[marriage3].element.marriage.previous=marriage;
				database[marriage].element.marriage.next=marriage3;
			}
		}
	}
	database[linked].element.person.marriage=marriage;
	database[unlinked].element.person.marriage=marriage;
	database[marriage].element.marriage.principal=linked;
	database[marriage].element.marriage.spouse=unlinked;
	database[marriage].element.marriage.leftchild=none;
	database[marriage].element.marriage.rightchild=none;
	Modules_ChangedStructure();
	return marriage;
}

void Database_AddChild(elementptr marriage,elementptr child)
{
	AJWLib_Assert(database!=NULL);
	database[child].element.person.siblingsltor=none;
	database[child].element.person.siblingsrtol=database[marriage].element.marriage.rightchild;
	database[marriage].element.marriage.rightchild=child;
	if (database[marriage].element.marriage.leftchild==none) database[marriage].element.marriage.leftchild=child;
	if (database[child].element.person.siblingsrtol!=none) database[database[child].element.person.siblingsrtol].element.person.siblingsltor=child;
	database[child].element.person.parentsmarriage=marriage;
	Modules_ChangedStructure();
}

static void Database_EditPerson(elementptr person)
{
	int i;
	
	AJWLib_Assert(database!=NULL);
	Desk_Icon_SetText(editpersonwin,editpersonicon_SURNAME,database[person].element.person.data.surname);
	Desk_Icon_SetText(editpersonwin,editpersonicon_FORENAME,database[person].element.person.data.forename);
	Desk_Icon_SetText(editpersonwin,editpersonicon_MIDDLENAMES,database[person].element.person.data.middlenames);
	switch (database[person].element.person.data.sex) {
		case sex_MALE:
			AJWLib_Msgs_SetText(editpersonwin,editpersonicon_SEX,"Sex.M:");
			break;
		case sex_FEMALE:
			AJWLib_Msgs_SetText(editpersonwin,editpersonicon_SEX,"Sex.F:");
			break;
		default:
			AJWLib_Msgs_SetText(editpersonwin,editpersonicon_SEX,"Sex.U:");
			break;
	}
	for (i=0;i<NUMBERPERSONUSERFIELDS;i++) {
		Desk_Icon_SetText(editpersonwin,editpersonicon_USERBASE+2*i,personuser[i]);
		Desk_Icon_SetText(editpersonwin,editpersonicon_USERBASE+1+2*i,database[person].element.person.data.user[i]);
	}
	Desk_Window_Show(editpersonwin,editingperson ? Desk_open_WHEREVER : Desk_open_CENTERED);
	Desk_Icon_SetCaret(editpersonwin,editpersonicon_SURNAME);
	editingperson=person;
}

char *Database_GetTitle(void)
{
	AJWLib_Assert(database!=NULL);
	return database[0].element.file.filetitle;
}

static void Database_EditTitle(void)
{
	Desk_Icon_SetText(edittitlewin,edittitleicon_TEXT,database[0].element.file.filetitle);
	Desk_Window_Show(edittitlewin,Desk_open_CENTEREDUNDERPOINTER);
	Desk_Icon_SetCaret(edittitlewin,edittitleicon_TEXT);
}

static Desk_bool Database_OkEditTitleWindow(Desk_event_pollblock *block,void *ref)
{
	char *title;

	Desk_UNUSED(ref);
	if (block->data.mouse.button.data.menu) return Desk_FALSE;
	title=Desk_Icon_GetTextPtr(edittitlewin,edittitleicon_TEXT);
	Database_UpdateField(database[0].element.file.filetitle,title);
	if (block->data.mouse.button.data.select) Desk_Window_Hide(edittitlewin);
	Modules_ChangedLayout();
	return Desk_TRUE;
}

static Desk_bool Database_CancelEditTitleWindow(Desk_event_pollblock *block,void *ref)
{
	Desk_UNUSED(ref);
	if (!block->data.mouse.button.data.select) return Desk_FALSE;
	Desk_Window_Hide(edittitlewin);
	return Desk_TRUE;
}

static Desk_bool Database_CancelEditWindow(Desk_event_pollblock *block,void *ref)
{
	Desk_UNUSED(ref);
	AJWLib_Assert(database!=NULL);
	if (!block->data.mouse.button.data.select) return Desk_FALSE;
	editingperson=none;
	Desk_Window_Hide(editpersonwin);
	return Desk_TRUE;
}

static Desk_bool Database_OkEditWindow(Desk_event_pollblock *block,void *ref)
{
	int i;
	char *icon;

	Desk_UNUSED(ref);
	AJWLib_Assert(database!=NULL);
	if (block->data.mouse.button.data.menu || editingperson==none) return Desk_FALSE;
	icon=Desk_Icon_GetTextPtr(editpersonwin,editpersonicon_SURNAME);
	Database_UpdateField(database[editingperson].element.person.data.surname,icon);
	icon=Desk_Icon_GetTextPtr(editpersonwin,editpersonicon_FORENAME);
	Database_UpdateField(database[editingperson].element.person.data.forename,icon);
	icon=Desk_Icon_GetTextPtr(editpersonwin,editpersonicon_MIDDLENAMES);
	Database_UpdateField(database[editingperson].element.person.data.middlenames,icon);
	if (!strcmp(Desk_Icon_GetTextPtr(editpersonwin,editpersonicon_SEX),AJWLib_Msgs_TempLookup("Sex.M:"))) database[editingperson].element.person.data.sex=sex_MALE;
	else if (!strcmp(Desk_Icon_GetTextPtr(editpersonwin,editpersonicon_SEX),AJWLib_Msgs_TempLookup("Sex.F:"))) database[editingperson].element.person.data.sex=sex_FEMALE;
	else database[editingperson].element.person.data.sex=sex_UNKNOWN;
	for (i=0;i<NUMBERPERSONUSERFIELDS;i++) {
		icon=Desk_Icon_GetTextPtr(editpersonwin,editpersonicon_USERBASE+1+2*i);
		Database_UpdateField(database[editingperson].element.person.data.user[i],icon);
	}
	Modules_ChangedData(editingperson);
	Graphics_PersonChanged(editingperson);
	editingperson=none;
	if (block->data.mouse.button.data.select) Desk_Window_Hide(editpersonwin);
	return Desk_TRUE;
}

static Desk_bool Database_CancelEditMarriageWindow(Desk_event_pollblock *block,void *ref)
{
	Desk_UNUSED(ref);
	AJWLib_Assert(database!=NULL);
	if (!block->data.mouse.button.data.select) return Desk_FALSE;
	editingmarriage=none;
	Desk_Window_Hide(editmarriagewin);
	return Desk_TRUE;
}

static Desk_bool Database_OkEditMarriageWindow(Desk_event_pollblock *block,void *ref)
{
	int i;
	char *icon;

	Desk_UNUSED(ref);
	AJWLib_Assert(database!=NULL);
	if (block->data.mouse.button.data.menu || editingmarriage==none) return Desk_FALSE;
	for (i=0;i<NUMBERMARRIAGEUSERFIELDS;i++) {
		icon=Desk_Icon_GetTextPtr(editmarriagewin,editmarriageicon_USERBASE+1+2*i);
		Database_UpdateField(database[editingmarriage].element.marriage.data.user[i],icon);
	}
	Modules_ChangedData(editingmarriage);
	Graphics_MarriageChanged(editingmarriage);
	editingmarriage=none;
	if (block->data.mouse.button.data.select) Desk_Window_Hide(editmarriagewin);
	return Desk_TRUE;
}

static void Database_EditMarriage(elementptr marriage)
{
	int i;

	AJWLib_Assert(database!=NULL);
	if (marriage==none) return;
	
	Desk_Icon_SetText(editmarriagewin,editmarriageicon_PRINCIPAL,Database_GetFullName(database[marriage].element.marriage.principal));
	Desk_Icon_SetText(editmarriagewin,editmarriageicon_SPOUSE,Database_GetFullName(database[marriage].element.marriage.spouse));
	for (i=0;i<NUMBERMARRIAGEUSERFIELDS;i++) {
		Desk_Icon_SetText(editmarriagewin,editmarriageicon_USERBASE+2*i,marriageuser[i]);
		Desk_Icon_SetText(editmarriagewin,editmarriageicon_USERBASE+1+2*i,database[marriage].element.marriage.data.user[i]);
	}
	Desk_Window_Show(editmarriagewin,editingmarriage ? Desk_open_WHEREVER : Desk_open_CENTERED);
	Desk_Icon_SetCaret(editmarriagewin,editmarriageicon_USERBASE+1);
	editingmarriage=marriage;
}

void Database_Edit(elementptr element)
{
	AJWLib_Assert(database!=NULL);

	switch (Database_GetElementType(element)) {
		case element_PERSON:
			Database_EditPerson(element);
			break;
		case element_MARRIAGE:
			Database_EditMarriage(element);
			break;
		case element_TITLE:
			Database_EditTitle();
			break;
		default:
			break;
	}
}

void Database_Delete(elementptr person)
{
	AJWLib_Assert(database!=NULL);
	Database_FreeElement(person);
	Modules_ChangedStructure();
}

elementptr Database_Add(void)
{
	elementptr newperson;
	int i;
	
	AJWLib_Assert(database!=NULL);
	newperson=Database_GetFreeElement(); /*Ok if an error is thrown*/
	database[newperson].type=element_PERSON;
	database[newperson].selected=Desk_FALSE;
	database[newperson].element.person.parentsmarriage=none;
	database[newperson].element.person.siblingsrtol=none;
	database[newperson].element.person.siblingsltor=none;
	database[newperson].element.person.marriage=none;
	for (i=0;i<NUMBERPERSONUSERFIELDS;i++) {
		database[newperson].element.person.data.user[i]=Desk_DeskMem_Malloc(1);
		database[newperson].element.person.data.user[i][0]='\0';
	}
	database[newperson].element.person.data.surname=Desk_DeskMem_Malloc(FIELDSIZE);
	sprintf(database[newperson].element.person.data.surname,"NewPerson%d",database[0].element.file.newpersonnumber++);
	database[newperson].element.person.data.forename=Desk_DeskMem_Malloc(1);
	database[newperson].element.person.data.forename[0]='\0';
	database[newperson].element.person.data.middlenames=Desk_DeskMem_Malloc(1);
	database[newperson].element.person.data.middlenames[0]='\0';
	database[newperson].element.person.data.sex=sex_UNKNOWN;
	Modules_ChangedStructure();
	return newperson;
}

elementptr Database_AddMarriage(void)
{
	elementptr newmarriage;
	int i;
	
	AJWLib_Assert(database!=NULL);
	newmarriage=Database_GetFreeElement(); /*Ok if an error is thrown*/
	database[newmarriage].type=element_MARRIAGE;
	database[newmarriage].selected=Desk_FALSE;
	database[newmarriage].element.marriage.next=none;
	database[newmarriage].element.marriage.previous=none;
	database[newmarriage].element.marriage.leftchild=none;
	database[newmarriage].element.marriage.rightchild=none;
	database[newmarriage].element.marriage.principal=none;
	database[newmarriage].element.marriage.spouse=none;
	for (i=0;i<NUMBERMARRIAGEUSERFIELDS;i++) {
		database[newmarriage].element.marriage.data.user[i]=Desk_DeskMem_Malloc(1);
		database[newmarriage].element.marriage.data.user[i][0]='\0';
	}

	Modules_ChangedStructure();
	return newmarriage;
}

static void Database_SexMenuClick(int entry,void *ref)
{
	Desk_UNUSED(ref);
	AJWLib_Assert(database!=NULL);
	Desk_Icon_SetText(editpersonwin,editpersonicon_SEX,Desk_Menu_GetText(sexmenu,entry));
}

int Database_GetNumPeople(void)
{
	int numpeople=0,i;
	AJWLib_Assert(database!=NULL);
	for (i=0;i<database[0].element.file.numberofelements;i++) {
		if (database[i].type==element_PERSON) numpeople++;
	}
	return numpeople;
}

static void Database_WriteTag(FILE *file,char *tag)
/*Write out a tag, utitlising any previous levels already written*/
{
	int level=0;
	static char previous[256]="";
	char *newtag,*old,*origtag;

	if (file==NULL) {
		/*Reset*/
		strcpy(previous,tag);
		return;
	}
	old=previous;
	newtag=origtag=tag;
	/* Skip all levels in the new tag that are also present in the old tag*/
	while (*old!='\0' && *newtag==*old) {
		if (*old=='.') {
			level++;
			tag=newtag+1;
		}
		newtag++;
		old++;
	}
	if (*newtag=='.' && *old=='\0') {
		level++;
		tag=newtag+1;
	}

	strcpy(previous,origtag);

	while (Desk_TRUE) {
		/*Write out the remaining levels*/
		fprintf(file,"%d ",level++);
		while (*tag!='\0' && *tag!='.') {
			fputc(*tag,file);
			tag++;
		}
		if (*tag=='.') {
			fputc('\n',file);
		} else {
			fputc(' ',file);
			break;
		}
		tag++;
	}
}

void Database_SaveGEDCOM(FILE *file,Desk_bool plainGEDCOM)
{
	elementptr marriage,child;
	int i,j;

	AJWLib_Assert(database!=NULL);
	AJWLib_Assert(file!=NULL);
	Database_WriteTag(NULL,"");
	if (!plainGEDCOM) {
		fprintf(file,"0 @F1@ _FILEINFO\n");
		fprintf(file,"1 _TITLE %s\n",database[0].element.file.filetitle);
		fprintf(file,"1 _NEXTNEWPERSON %d\n",database[0].element.file.newpersonnumber);
		for (i=0;i<NUMBERPERSONUSERFIELDS;i++) {
			fprintf(file,"1 _PERSONUSER %d\n",i);
			fprintf(file,"2 _DESC %s\n",personuser[i]);
			fprintf(file,"2 _GEDCOM %s\n",personGEDCOM[i]);
		}
		for (i=0;i<NUMBERMARRIAGEUSERFIELDS;i++) {
			fprintf(file,"1 _MARRIAGEUSER %d\n",i);
			fprintf(file,"2 _DESC %s\n",marriageuser[i]);
			fprintf(file,"2 _GEDCOM %s\n",marriageGEDCOM[i]);
		}
	}
	for (i=1;i<database[0].element.file.numberofelements;i++) {
		switch (database[i].type) {
			case element_PERSON:
				fprintf(file,"0 @%d@ INDI\n",i);
				fprintf(file,"1 NAME %s %s/%s/\n",database[i].element.person.data.forename,database[i].element.person.data.middlenames,database[i].element.person.data.surname);
				if (database[i].element.person.data.sex!=sex_UNKNOWN) fprintf(file,"1 SEX %c\n",database[i].element.person.data.sex);
				Database_WriteTag(NULL,"INDI");
				for (j=0;j<NUMBERPERSONUSERFIELDS;j++) {
					if (strcmp(database[i].element.person.data.user[j],"")!=0 && strcmp(personGEDCOM[j],"")!=0) {
						Database_WriteTag(file,personGEDCOM[j]);
						fprintf(file,"%s\n",database[i].element.person.data.user[j]);
					}
				}
				if (database[i].element.person.marriage) {
					marriage=database[i].element.person.marriage;
					while (marriage) {
						if (database[marriage].element.marriage.principal==i || database[marriage].element.marriage.spouse==i) {
							fprintf(file,"1 FAMS @%d@\n",marriage);
						}
						marriage=database[marriage].element.marriage.next;
					}
					if (database[i].element.person.marriage) {
						marriage=database[database[i].element.person.marriage].element.marriage.previous;
						while (marriage) {
							if (database[marriage].element.marriage.principal==i || database[marriage].element.marriage.spouse==i) {
								fprintf(file,"1 FAMS @%d@\n",marriage);
							}
							marriage=database[marriage].element.marriage.previous;
						}
					}
				}
				if (database[i].element.person.parentsmarriage) {
					fprintf(file,"1 FAMC @%d@\n",database[i].element.person.parentsmarriage);
				}
				break;
			case element_MARRIAGE:
				fprintf(file,"0 @%d@ FAM\n",i);
				fprintf(file,"1 HUSB @%d@\n",database[i].element.marriage.principal);
				fprintf(file,"1 WIFE @%d@\n",database[i].element.marriage.spouse);
				child=database[i].element.marriage.leftchild;
				while (child) {
					fprintf(file,"1 CHIL @%d@\n",child);
					child=database[child].element.person.siblingsltor;
				}
				Database_WriteTag(NULL,"FAM");
				for (j=0;j<NUMBERMARRIAGEUSERFIELDS;j++) {
					if (strcmp(database[i].element.marriage.data.user[j],"")!=0 && strcmp(marriageGEDCOM[j],"")!=0) {
						Database_WriteTag(file,marriageGEDCOM[j]);
						fprintf(file,"%s\n",database[i].element.marriage.data.user[j]);
					}
				}
				break;
			default:
				break;
		}
	}
}

char *Database_GetPersonUserDesc(int num)
{
	AJWLib_Assert(num>=0);
	AJWLib_Assert(num<NUMBERPERSONUSERFIELDS);
	return personuser[num];
}

void Database_SetPersonUserDesc(int num,char *desc)
{
	AJWLib_Assert(num>=0);
	AJWLib_Assert(num<NUMBERPERSONUSERFIELDS);
	strcpy(personuser[num],desc);
}

char *Database_GetPersonGEDCOMDesc(int num)
{
	AJWLib_Assert(num>=0);
	AJWLib_Assert(num<NUMBERPERSONUSERFIELDS);
	return personGEDCOM[num];
}

void Database_SetPersonGEDCOMDesc(int num,char *desc)
{
	AJWLib_Assert(num>=0);
	AJWLib_Assert(num<NUMBERPERSONUSERFIELDS);
	strcpy(personGEDCOM[num],desc);
}

char *Database_GetMarriageUserDesc(int num)
{
	AJWLib_Assert(num>=0);
	AJWLib_Assert(num<NUMBERMARRIAGEUSERFIELDS);
	return marriageuser[num];
}

void Database_SetMarriageUserDesc(int num,char *desc)
{
	AJWLib_Assert(num>=0);
	AJWLib_Assert(num<NUMBERMARRIAGEUSERFIELDS);
	strcpy(marriageuser[num],desc);
}

char *Database_GetMarriageGEDCOMDesc(int num)
{
	AJWLib_Assert(num>=0);
	AJWLib_Assert(num<NUMBERMARRIAGEUSERFIELDS);
	return marriageGEDCOM[num];
}

void Database_SetMarriageGEDCOMDesc(int num,char *desc)
{
	AJWLib_Assert(num>=0);
	AJWLib_Assert(num<NUMBERMARRIAGEUSERFIELDS);
	strcpy(marriageGEDCOM[num],desc);
}

void Database_New(void)
{
	int i;
	char *title;

	AJWLib_Assert(database==NULL);
	AJWLib_Flex_Alloc((flex_ptr)&database,sizeof(databaseelement));
	database[0].type=element_FILE;
	database[0].element.file.numberofelements=1;
	database[0].element.file.newpersonnumber=1;
	database[0].element.file.freeelement=0;
	title=AJWLib_Msgs_TempLookup("Tree.Title:Title");
	database[0].element.file.filetitle=Desk_DeskMem_Malloc(strlen(title)+1);
	strcpy(database[0].element.file.filetitle,title);
	for (i=0;i<NUMBERPERSONUSERFIELDS;i++) {
		char tag[20], *buffer;

		sprintf(tag,"User.P%d:",i+1);
		buffer=AJWLib_Msgs_TempLookup(tag);
		if (strcmp(buffer,"*")==0) strcpy(personuser[i],""); else strcpy(personuser[i],buffer);
		sprintf(tag,"GED.P%d:",i+1);
		buffer=AJWLib_Msgs_TempLookup(tag);
		if (strcmp(buffer,"*")==0) strcpy(personGEDCOM[i],""); else strcpy(personGEDCOM[i],buffer);
	}
	for (i=0;i<NUMBERMARRIAGEUSERFIELDS;i++) {
		char tag[20], *buffer;

		sprintf(tag,"User.M%d:",i+1);
		buffer=AJWLib_Msgs_TempLookup(tag);
		if (strcmp(buffer,"*")==0) strcpy(marriageuser[i],""); else strcpy(marriageuser[i],buffer);
		sprintf(tag,"GED.M%d:",i+1);
		buffer=AJWLib_Msgs_TempLookup(tag);
		if (strcmp(buffer,"*")==0) strcpy(marriageGEDCOM[i],""); else strcpy(marriageGEDCOM[i],buffer);
	}
}

void Database_StopEditing(void)
{
	Desk_Window_Hide(edittitlewin);
	Desk_Window_Hide(editpersonwin);
	Desk_Window_Hide(editmarriagewin);
	editingperson=none;
	editingmarriage=none;
}

void Database_Remove(void)
{
	AJWLib_AssertWarning(database!=NULL);
	if (database) AJWLib_Flex_Free((flex_ptr)&database);
	Database_StopEditing();
}

Desk_bool Database_Loaded(void)
{
	return database==NULL ? Desk_FALSE : Desk_TRUE;
}

void Database_Init(void)
{
	AJWLib_Assert(database==NULL);
	editpersonwin=Desk_Window_Create("EditPerson",Desk_template_TITLEMIN);
	editmarriagewin=Desk_Window_Create("EditMarriage",Desk_template_TITLEMIN);
	edittitlewin=Desk_Window_Create("EditTitle",Desk_template_TITLEMIN);
	Desk_Event_Claim(Desk_event_CLICK,editpersonwin,editpersonicon_OK,Database_OkEditWindow,NULL);
	Desk_Event_Claim(Desk_event_CLICK,editpersonwin,editpersonicon_CANCEL,Database_CancelEditWindow,NULL);
	Desk_Event_Claim(Desk_event_CLICK,editmarriagewin,editmarriageicon_OK,Database_OkEditMarriageWindow,NULL);
	Desk_Event_Claim(Desk_event_CLICK,editmarriagewin,editmarriageicon_CANCEL,Database_CancelEditMarriageWindow,NULL);
	Desk_Event_Claim(Desk_event_CLICK,edittitlewin,edittitleicon_OK,Database_OkEditTitleWindow,NULL);
	Desk_Event_Claim(Desk_event_CLICK,edittitlewin,edittitleicon_CANCEL,Database_CancelEditTitleWindow,NULL);
	AJWLib_Window_KeyHandler(edittitlewin,edittitleicon_OK,Database_OkEditTitleWindow,edittitleicon_CANCEL,Database_CancelEditTitleWindow,NULL);
	AJWLib_Window_KeyHandler(editpersonwin,editpersonicon_OK,Database_OkEditWindow, editpersonicon_CANCEL,Database_CancelEditWindow,NULL);
	AJWLib_Window_KeyHandler(editmarriagewin,editmarriageicon_OK,Database_OkEditMarriageWindow,editmarriageicon_CANCEL,Database_CancelEditMarriageWindow,NULL);
	sexmenu=AJWLib_Menu_CreateFromMsgs("Title.Sex:","Menu.Sex:M,F,U",Database_SexMenuClick,NULL);
	AJWLib_Menu_AttachPopup(editpersonwin,editpersonicon_SEXMENU,editpersonicon_SEX,sexmenu,Desk_button_MENU | Desk_button_SELECT);
}

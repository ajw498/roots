/*
	Roots - Database
	© Alex Waugh 1999

	$Id: Database.c,v 1.44 2000/10/05 14:46:35 AJW Exp $

*/


#include "Desk.Window.h"
#include "Desk.Error2.h"
#include "Desk.Event.h"
#include "Desk.Template.h"
#include "Desk.File.h"
#include "Desk.Menu.h"
#include "Desk.DeskMem.h"
#include "Desk.Filing.h"

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
	char surname[FIELDSIZE];
	char forename[FIELDSIZE];
	char middlenames[FIELDSIZE];
	sextype sex;
    char user[NUMBERPERSONUSERFIELDS][FIELDSIZE];
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
	char user[NUMBERMARRIAGEUSERFIELDS][FIELDSIZE];
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
	char filetitle[FIELDSIZE];
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

elementptr Database_GetLinked(int *index)
/* Get the next person in the database*/
{
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
	while ((++(*index))<database[0].element.file.numberofelements) {
		if (database[*index].type==element_MARRIAGE) {
			return *index;
		}
	}
	*index=0;
	return none;
}

void Database_SetTitle(char *title)
{
	strncpy(database[0].element.file.filetitle,title,FIELDSIZE-1);
	database[0].element.file.filetitle[FIELDSIZE-1]='\0';
}

void Database_SetNextNewPerson(int personnumber)
{
	database[0].element.file.newpersonnumber=personnumber;
}

void Database_SetForename(elementptr person,char *name)
{
	strncpy(database[person].element.person.data.forename,name,FIELDSIZE-1);
	database[person].element.person.data.forename[FIELDSIZE-1]='\0';
}

void Database_SetMiddleNames(elementptr person,char *name)
{
	strncpy(database[person].element.person.data.middlenames,name,FIELDSIZE-1);
	database[person].element.person.data.middlenames[FIELDSIZE-1]='\0';
}

void Database_SetSurname(elementptr person,char *name)
{
	strncpy(database[person].element.person.data.surname,name,FIELDSIZE-1);
	database[person].element.person.data.surname[FIELDSIZE-1]='\0';
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
	strncpy(database[person].element.person.data.user[num],str,FIELDSIZE-1);
	database[person].element.person.data.user[num][FIELDSIZE-1]='\0';
}

void Database_SetMarriageUser(int num,elementptr marriage,char *str)
{
	AJWLib_Assert(num<NUMBERMARRIAGEUSERFIELDS);
	strncpy(database[marriage].element.marriage.data.user[num],str,FIELDSIZE-1);
	database[marriage].element.marriage.data.user[num][FIELDSIZE-1]='\0';
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

static void Database_CheckElement(elementptr person,elementptr not,elementtype type)
/* Check that the given elementptr is a valid person, and is not the same as not*/
{
	if (person==not) AJWLib_Error2_HandleMsgs("Error.Dizzy:");
	if (person<0 || person>=database[0].element.file.numberofelements) AJWLib_Error2_HandleMsgs("Error.Elvis:");
	if (person) if (database[person].type!=type) AJWLib_Error2_HandleMsgs("Error.Alien:");
}

void Database_CheckConsistency(void)
/* Check for invalid links between people*/
{
	int i;
	
	AJWLib_Assert(database!=NULL);
	for (i=1;i<database[0].element.file.numberofelements;i++) {
		switch (database[i].type) {
			case element_PERSON:
				Database_CheckElement(database[i].element.person.marriage,i,element_MARRIAGE);
				Database_CheckElement(database[i].element.person.parentsmarriage,i,element_MARRIAGE);
				Database_CheckElement(database[i].element.person.siblingsltor,i,element_PERSON);
				Database_CheckElement(database[i].element.person.siblingsltor,i,element_PERSON);
				break;
			case element_MARRIAGE:
				Database_CheckElement(database[i].element.marriage.next,i,element_MARRIAGE);
				Database_CheckElement(database[i].element.marriage.previous,i,element_MARRIAGE);
				Database_CheckElement(database[i].element.marriage.principal,i,element_PERSON);
				Database_CheckElement(database[i].element.marriage.spouse,i,element_PERSON);
				Database_CheckElement(database[i].element.marriage.leftchild,i,element_PERSON);
				Database_CheckElement(database[i].element.marriage.rightchild,i,element_PERSON);
				break;
			case element_FREE:
				Database_CheckElement(database[i].element.freeelement.next,i,element_FREE);
				break;
			case element_FILE:
				Database_CheckElement(database[i].element.file.freeelement,-1,element_FREE);
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

static void Database_FreeElement(elementptr element)
{
	AJWLib_Assert(database!=NULL);
	database[element].element.freeelement.next=database[0].element.file.freeelement;
	database[0].element.file.freeelement=element;
	database[element].type=element_FREE;
	database[element].selected=Desk_FALSE;
}

void Database_Select(elementptr person)
{
	AJWLib_Assert(database!=NULL);
	AJWLib_Assert(person!=none);
	database[person].selected=Desk_TRUE;
}

void Database_DeSelect(elementptr person)
{
	AJWLib_Assert(database!=NULL);
	AJWLib_Assert(person!=none);
	database[person].selected=Desk_FALSE;
}

void Database_DeSelectAll(void)
{
	int i;
	AJWLib_Assert(database!=NULL);
	for (i=0;i<database[0].element.file.numberofelements;i++) {
		if (database[i].type==element_PERSON || database[i].type==element_MARRIAGE) {
			database[i].selected=Desk_FALSE;
		}
	}
}

elementtype Database_AnyoneSelected(void)
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

Desk_bool Database_GetSelect(elementptr person)
{
	AJWLib_Assert(database!=NULL);
	AJWLib_Assert(person!=none);
	return database[person].selected;
}

void Database_RemoveMarriage(elementptr marriage)
{
	elementptr child;
	/*Remove marriage from chain*/
	if (database[marriage].element.marriage.previous==none) {
		database[database[marriage].element.marriage.principal].element.person.marriage=database[marriage].element.marriage.next;
		if (database[marriage].element.marriage.next) {
			database[database[marriage].element.marriage.next].element.marriage.previous=none;
		}
	} else {
		database[database[marriage].element.marriage.previous].element.marriage.next=database[marriage].element.marriage.next;
		if (database[marriage].element.marriage.next) {
			database[database[marriage].element.marriage.next].element.marriage.previous=database[marriage].element.marriage.previous;
		}
	}
	/*Remove marriage from spouse*/
	database[database[marriage].element.marriage.spouse].element.person.marriage=none;
	/*Unlink any children from this marriage and their siblings*/
	child=database[marriage].element.marriage.leftchild;
	while (child) {
		int temp;
		database[child].element.person.parentsmarriage=none;
		database[child].element.person.siblingsrtol=none;
		temp=database[child].element.person.siblingsltor;
		database[child].element.person.siblingsltor=none;
		child=temp;
	}
	Database_FreeElement(marriage);
}

void Database_UnlinkSelected(layout *layout)
{
	int i;
	elementptr marriage;
	AJWLib_Assert(database!=NULL);
	for (i=1;i<database[0].element.file.numberofelements;i++) {
		switch (database[i].type) {
			case element_PERSON:
				/*Get parents marriage*/
				if ((marriage=Database_GetParentsMarriage(i))!=none) {
					/*If selection differs from parents marriage then unlink from siblings and from parents*/
					if (database[marriage].selected!=database[i].selected) {
						/*Remove from siblings chain*/
						if (database[i].element.person.siblingsltor) database[database[i].element.person.siblingsltor].element.person.siblingsrtol=database[i].element.person.siblingsrtol;
						if (database[i].element.person.siblingsrtol) database[database[i].element.person.siblingsrtol].element.person.siblingsltor=database[i].element.person.siblingsltor;
						/*Remove from parents marriage*/
						if (database[marriage].element.marriage.leftchild==i) database[marriage].element.marriage.leftchild=database[i].element.person.siblingsltor;
						if (database[marriage].element.marriage.rightchild==i) database[marriage].element.marriage.rightchild=database[i].element.person.siblingsrtol;
						/*Remove parents*/
						database[i].element.person.parentsmarriage=none;
					}
				}
				break;
			case element_MARRIAGE:
				/*Remove marriage if both spouses and the marriage are not the same selection*/
				if (database[database[i].element.marriage.principal].selected!=database[i].selected || database[database[i].element.marriage.spouse].selected!=database[i].selected) {
					Layout_RemoveMarriage(layout,i);
					Database_RemoveMarriage(i);
					Modules_ChangedStructure();
					break;
				}
			default:
				break;
		}
	}
}

void Database_DeleteSelected(layout *layout)
{
	int i;
	AJWLib_Assert(database!=NULL);
	Database_UnlinkSelected(layout);
	for (i=1;i<database[0].element.file.numberofelements;i++) {
		if (database[i].selected) {
			switch (database[i].type) {
				case element_PERSON:
					Layout_RemovePerson(layout,i);
					Database_FreeElement(i);
					Modules_ChangedStructure();
					break;
				case element_MARRIAGE:
					Layout_RemoveMarriage(layout,i);
					Database_FreeElement(i);
					Modules_ChangedStructure();
					break;
				default:
					break;
			}
		}
	}
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
	if (person==none) return none;
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
			while (database[marriage3].element.marriage.previous!=none) marriage3=database[marriage3].element.marriage.previous;
			database[marriage2].element.marriage.next=marriage;
			database[marriage].element.marriage.previous=marriage2;
			database[marriage3].element.marriage.previous=marriage;
			database[marriage].element.marriage.next=marriage3;
		}
	}
	database[linked].element.person.marriage=marriage;
	database[unlinked].element.person.marriage=marriage;
	database[marriage].element.marriage.principal=linked;
	database[marriage].element.marriage.spouse=unlinked;
	database[marriage].element.marriage.leftchild=none;
	database[marriage].element.marriage.rightchild=none;
	for (i=0;i<NUMBERMARRIAGEUSERFIELDS;i++) {
		strcpy(database[marriage].element.marriage.data.user[i],"");
	}
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

void Database_EditTitle(void)
{
	Desk_Icon_SetText(edittitlewin,edittitleicon_TEXT,database[0].element.file.filetitle);
	Desk_Window_Show(edittitlewin,Desk_open_CENTEREDUNDERPOINTER);
	Desk_Icon_SetCaret(edittitlewin,edittitleicon_TEXT);
}

static Desk_bool Database_OkEditTitleWindow(Desk_event_pollblock *block,void *ref)
{
	Desk_UNUSED(ref);
	if (block->data.mouse.button.data.menu) return Desk_FALSE;
	Desk_Icon_GetText(edittitlewin,edittitleicon_TEXT,database[0].element.file.filetitle);
	if (block->data.mouse.button.data.select) Desk_Window_Hide(edittitlewin);
	Modules_ChangedData(none);
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
	
	Desk_UNUSED(ref);
	AJWLib_Assert(database!=NULL);
	if (block->data.mouse.button.data.menu || editingperson==none) return Desk_FALSE;
	Desk_Icon_GetText(editpersonwin,editpersonicon_SURNAME,database[editingperson].element.person.data.surname);
	Desk_Icon_GetText(editpersonwin,editpersonicon_FORENAME,database[editingperson].element.person.data.forename);
	Desk_Icon_GetText(editpersonwin,editpersonicon_MIDDLENAMES,database[editingperson].element.person.data.middlenames);
	if (!strcmp(Desk_Icon_GetTextPtr(editpersonwin,editpersonicon_SEX),AJWLib_Msgs_TempLookup("Sex.M:"))) database[editingperson].element.person.data.sex=sex_MALE;
	else if (!strcmp(Desk_Icon_GetTextPtr(editpersonwin,editpersonicon_SEX),AJWLib_Msgs_TempLookup("Sex.F:"))) database[editingperson].element.person.data.sex=sex_FEMALE;
	else database[editingperson].element.person.data.sex=sex_UNKNOWN;
	for (i=0;i<NUMBERPERSONUSERFIELDS;i++) {
		Desk_Icon_GetText(editpersonwin,editpersonicon_USERBASE+1+2*i,database[editingperson].element.person.data.user[i]);
	}
	Modules_ChangedData(editingperson);
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

	Desk_UNUSED(ref);
	AJWLib_Assert(database!=NULL);
	if (block->data.mouse.button.data.menu || editingmarriage==none) return Desk_FALSE;
	for (i=0;i<NUMBERMARRIAGEUSERFIELDS;i++) {
		Desk_Icon_GetText(editmarriagewin,editmarriageicon_USERBASE+1+2*i,database[editingmarriage].element.marriage.data.user[i]);
	}
	Modules_ChangedData(editingmarriage);
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

void Database_Edit(elementptr person)
{
	AJWLib_Assert(database!=NULL);
	AJWLib_Assert(person!=none);
	switch (database[person].type) {
		case element_PERSON:
			Database_EditPerson(person);
			break;
		case element_MARRIAGE:
			Database_EditMarriage(person);
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
	sprintf(database[newperson].element.person.data.surname,"NewPerson%d",database[0].element.file.newpersonnumber++);
	strcpy(database[newperson].element.person.data.forename,"");
	strcpy(database[newperson].element.person.data.middlenames,"");
	database[newperson].element.person.data.sex=sex_UNKNOWN;
	for (i=0;i<NUMBERPERSONUSERFIELDS;i++) {
		strcpy(database[newperson].element.person.data.user[i],"");
	}
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
	for (i=0;i<NUMBERPERSONUSERFIELDS;i++) {
		strcpy(database[newmarriage].element.marriage.data.user[i],"");
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
	char *new,*old,*origtag;

	if (file==NULL) {
		/*Reset*/
		strcpy(previous,tag);
		return;
	}
	old=previous;
	new=origtag=tag;
	/* Skip all levels in the new tag that are also present in the old tag*/
	while (*old!='\0' && *new==*old) {
		if (*old=='.') {
			level++;
			tag=new+1;
		}
		new++;
		old++;
	}
	if (*new=='.' && *old=='\0') {
		level++;
		tag=new+1;
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

	AJWLib_Assert(database==NULL);
	AJWLib_Flex_Alloc((flex_ptr)&database,sizeof(databaseelement));
	database[0].type=element_FILE;
	database[0].element.file.numberofelements=1;
	database[0].element.file.newpersonnumber=1;
	database[0].element.file.freeelement=0;
	strcpy(database[0].element.file.filetitle,AJWLib_Msgs_TempLookup("Tree.Title:Title"));
	for (i=0;i<NUMBERPERSONUSERFIELDS;i++) {
		char tag[20], *buffer;

		sprintf(tag,"User.P%d:",i+1);
		buffer=AJWLib_Msgs_TempLookup(tag);
		if (!strcmp(buffer,"*")) strcpy(buffer,"");
		strcpy(personuser[i],buffer);
		sprintf(tag,"GED.P%d:",i+1);
		buffer=AJWLib_Msgs_TempLookup(tag);
		if (!strcmp(buffer,"*")) strcpy(buffer,"");
		strcpy(personGEDCOM[i],buffer);
	}
	for (i=0;i<NUMBERMARRIAGEUSERFIELDS;i++) {
		char tag[20], *buffer;

		sprintf(tag,"User.M%d:",i+1);
		buffer=AJWLib_Msgs_TempLookup(tag);
		if (!strcmp(buffer,"*")) strcpy(buffer,"");
		strcpy(marriageuser[i],buffer);
		sprintf(tag,"GED.M%d:",i+1);
		buffer=AJWLib_Msgs_TempLookup(tag);
		if (!strcmp(buffer,"*")) strcpy(buffer,"");
		strcpy(marriageGEDCOM[i],buffer);
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

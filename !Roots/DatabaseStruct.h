#ifndef DATABASE_H
#define DATABASE_H

#include "Desk.Window.h"
#include "Layout.h"

#define none 0


#ifndef ELEMENTPTR
#define ELEMENTPTR

typedef int elementptr;

#endif


typedef enum elementtype {
	element_NONE,
	element_PERSON,
	element_MARRIAGE,
	element_SELECTION,
	element_FREE,
	element_FILE,
	element_TITLE
} elementtype;

typedef enum sextype {
	sex_MALE='M',
	sex_FEMALE='F',
	sex_UNKNOWN='U',
	sex_ANY='A'
} sextype;

typedef char date[20];

typedef struct persondata {
	char surname[20];
	char forename[20];
	char middlenames[20];
	char title[10];
	sextype sex;
	date dob;
	date dod;
    char placeofbirth[20];
    char userdata[3][20];
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
	char place[20];
	date date;
	date divorce;
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
	char filetitle[40];
	elementptr reserved0;
	elementptr freeelement;
	elementptr reserved1;
    char userdesc[3][20];
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

void Database_Select(elementptr person);
void Database_DeSelect(elementptr person);
void Database_DeSelectAll(void);
Desk_bool Database_GetSelect(elementptr person);
elementtype Database_AnyoneSelected(void);
void Database_UnlinkSelected(layout *layout);
void Database_DeleteSelected(layout *layout);
persondata *Database_GetPersonData(elementptr person);
elementptr Database_GetMarriage(elementptr person);
elementptr Database_GetMarriageLtoR(elementptr person);
elementptr Database_GetMarriageRtoL(elementptr person);
marriagedata *Database_GetMarriageData(elementptr marriage);
char *Database_GetName(elementptr person);
char *Database_GetFullName(elementptr person);
elementptr Database_GetMother(elementptr person);
elementptr Database_GetFather(elementptr person);
elementptr Database_GetParentsMarriage(elementptr person);
elementptr Database_GetSiblingLtoR(elementptr person);
elementptr Database_GetSiblingRtoL(elementptr person);
elementptr Database_GetLeftChild(elementptr person);
elementptr Database_GetRightChild(elementptr person);
elementptr Database_GetSpouseFromMarriage(elementptr marriage);
elementptr Database_GetPrincipalFromMarriage(elementptr marriage);
Desk_bool Database_IsFirstMarriage(elementptr marriage);
void Database_RemoveMarriage(elementptr marriage);
void Database_New(void);
void Database_Remove(void);
void Database_Init(void);
void Database_Save(FILE *file);
void Database_Load(FILE *file);
char *Database_GetTitle(void);
elementptr Database_Marry(elementptr linked,elementptr unlinked);
void Database_AddChild(elementptr marriage,elementptr child);
void Database_EditTitle(void);
void Database_Edit(elementptr person);
elementptr Database_Add(void);
void Database_Delete(elementptr person);
int Database_GetNumPeople(void);
int Database_GetSize(void);
void Database_StopEditing(void);
char *Database_GetUserDesc(int num);
void Database_SetUserDesc(int num,char *desc);
sextype Database_GetSex(elementptr person);


#endif


#ifndef DATABASE_H
#define DATABASE_H

#include "Desk.Window.h"

#define none 0


typedef int elementptr;

typedef enum sex {
	male='M',
	female='F',
	unknown='U'
} sex;

typedef union {
	char string[9];
	struct {
		char day[2];
		char month[2];
		char year[4];
		char zero;
	} data;
} date;

typedef struct persondata {
	char surname[20];
	char forename[20];
	char middlenames[20];
	char title[10];
	sex  sex;
	date dob;
	date dod;
    char placeofbirth[20];
    char userdata[3][20];
/*    char notes[200];*/
} persondata;

typedef struct person {
	elementptr mother;
	elementptr father;
	elementptr siblingsrtol;
	elementptr siblingsltor;
	elementptr marriage;
	elementptr nextunlinked;
	persondata data;
} person;

typedef struct freeelement {
	elementptr next;
} freeelement;

typedef struct marriagedata {
	char place[20];
	date date;
	date end;
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
	char filetitle[20];
	char filedescription[200];
	elementptr unlinkedpeople;
	elementptr freeelement;
	elementptr linkedpeople;
    char userdesc[3][20];
} file;

typedef union element {
	person person;
	freeelement freeelement;
	marriage marriage;
	file file;
} element;

persondata *Database_GetPersonData(elementptr person);
elementptr Database_GetMarriage(elementptr person);
elementptr Database_GetMarriageLtoR(elementptr person);
elementptr Database_GetMarriageRtoL(elementptr person);
marriagedata *Database_GetMarriageData(elementptr marriage);
elementptr Database_GetUnlinked(elementptr person);
elementptr Database_GetLinked(void);
char *Database_GetName(elementptr person);
char *Database_GetFullName(elementptr person);
char *Database_GetTitledName(elementptr person);
char *Database_GetTitledFullName(elementptr person);
void Database_FudgeLinked(elementptr person);
void Database_FudgeMarriageSwap(elementptr marriage);
elementptr Database_GetMother(elementptr person);
elementptr Database_GetFather(elementptr person);
elementptr Database_GetSiblingLtoR(elementptr person);
elementptr Database_GetSiblingRtoL(elementptr person);
elementptr Database_GetLeftChild(elementptr person);
elementptr Database_GetRightChild(elementptr person);
elementptr Database_GetSpouseFromMarriage(elementptr marriage);
elementptr Database_GetPrincipalFromMarriage(elementptr marriage);
Desk_bool Database_IsFirstMarriage(elementptr marriage);
Desk_bool Database_IsUnlinked(elementptr person);
void Database_Info(Desk_window_handle infowin);
void Database_New(void);
void Database_Remove(void);
void Database_Init(void);
void Database_Save(FILE *file);
void Database_Load(FILE *file);
char *Database_GetTitle(void);
char *Database_GetDescription(void);
void Database_Marry(elementptr linked,elementptr unlinked);
void Database_AddChild(elementptr marriage,elementptr child);
void Database_AddParents(elementptr child,elementptr mother,elementptr father);
void Database_LinkPerson(elementptr person);
void Database_EditPerson(elementptr person);
void Database_EditMarriage(elementptr marriage);
void Database_RemoveChild(elementptr child);
void Database_RemoveSpouse(elementptr person);
void Database_RemoveParents(elementptr person);
void Database_Add(void);
void Database_Delete(elementptr person);
int Database_GetNumPeople(void);
int Database_GetSize(void);

#endif


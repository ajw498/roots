#ifndef DATABASESTRUCT_H
#define DATABASESTRUCT_H

#define none 0
#define FIELDSIZE 40
#define NUMBERPERSONUSERFIELDS 10
#define NUMBERMARRIAGEUSERFIELDS 5

typedef int elementptr;

typedef enum elementtype {
	element_TITLE=-5,
	element_LINE,
	element_NONE=0,
	element_PERSON,
	element_MARRIAGE,
	element_SELECTION,
	element_FREE,
	element_FILE
} elementtype;

typedef enum sextype {
	sex_MALE='M',
	sex_FEMALE='F',
	sex_UNKNOWN='U',
	sex_ANY='A'
} sextype;

#endif


#ifndef WINDOWSSTRUCT_H
#define WINDOWSSTRUCT_H

#include "DatabaseStruct.h"


typedef enum wintype {
	wintype_UNKNOWN=0,
	wintype_NORMAL,
	wintype_DESCENDENTS,
	wintype_ANCESTORS,
	wintype_CLOSERELATIVES
} wintype;

typedef struct windowdata {
	Desk_window_handle handle;
	wintype type;
	elementptr person;
	int generations;
	layout *layout;
	int scale;
} windowdata;

#endif

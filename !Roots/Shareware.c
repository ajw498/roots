/*
	Roots - Shareware stuff
	© Alex Waugh 1999

	$Id: Shareware.c,v 1.1 2000/09/22 13:11:00 AJW Exp $
	
*/


#ifndef SHAREWARE_STANDALONE

#include "Desk.Window.h"
#include "Desk.Error2.h"
#include "Desk.Msgs.h"

#include "AJWLib.Msgs.h"
#include "AJWLib.Error2.h"

#include "Main.h"
#include "Config.h"

#endif

#include "Shareware.h"

#include <string.h>
#include <stdio.h>

static unsigned int Shareware_CalcValue(char *user)
{
	unsigned int value=0x00000000;

	return value;
}

#ifndef SHAREWARE_STANDALONE

static Desk_bool registered=Desk_FALSE,errorgiven=Desk_FALSE;

char *Shareware_GetUser(void)
/* Initialise everything, and return the users name if registered*/
{
	char buffer[256];
	static char username[256]="Everyone";
	registered=Desk_TRUE;
	return username;
}

void Shareware_CheckRegistered(void)
/* Check that the user is registered, cause an error if they are not*/
{
	if (!registered) {
		errorgiven=Desk_TRUE;
		AJWLib_Error2_HandleMsgs("Error.Share:You cannot create any more people in the unregistered version");
	}
}

Desk_bool Shareware_GetErrorStatus(void)
/* See if there has just been an error generated*/
{
	if (!errorgiven) return Desk_FALSE;
	errorgiven=Desk_FALSE;
	return Desk_TRUE;
}

#else

int main(void)
{
	char buffer[256];

	printf("Enter user name\n");
	fgets(buffer,255,stdin);
	printf("\n\nValue is %u\n",Shareware_CalcValue(buffer));
	return 0;
}

#endif

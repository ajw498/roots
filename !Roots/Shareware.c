/*
	Roots - Shareware stuff
	© Alex Waugh 1999

	$Id: Shareware.c,v 1.3 2000/12/12 19:24:52 AJW Exp $
	
*/


#ifndef SHAREWARE_STANDALONE

#include "Desk/Window.h"
#include "Desk/Error2.h"
#include "Desk/Msgs.h"

#include "AJWLib/Msgs.h"
#include "AJWLib/Error2.h"

#include "Main.h"
#include "Config.h"

#endif

#include "Shareware.h"

#include <string.h>
#include <stdio.h>

static unsigned int Shareware_CalcValue(char *user)
{
	unsigned int value=0x0AA00AA0;

	while (*user) value= (value>>8) | (((value & 0xFF) ^ (*user++))<<24);
	value|=0x01000000;
	value^=0x02000000;
	return value;
}

#ifndef SHAREWARE_STANDALONE

static Desk_bool registered=Desk_FALSE,errorgiven=Desk_FALSE;

char *Shareware_GetUser(void)
/* Initialise everything, and return the users name if registered*/
{
	char buffer[256];
	static char username[256]="**Unregistered**";
	FILE *file;

	sprintf(buffer,"%s.User",choicesread);
	file=fopen(buffer,"r");
	if (file) {
		unsigned int calcval,actualval;
		fgets(buffer,255,file);
		calcval=Shareware_CalcValue(buffer);
		fscanf(file,"%u",&actualval);
		fclose(file);
		if (calcval==actualval) {
			registered=Desk_TRUE;
			strcpy(username,buffer);
		}
	}
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

	fprintf(stderr,"Enter user name\n");
	fgets(buffer,255,stdin);
	printf("%s%u\n",buffer,Shareware_CalcValue(buffer));
	return 0;
}

#endif

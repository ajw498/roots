/*
	FT - Configuration
	© Alex Waugh 1999

	$Log: Config.c,v $
	Revision 1.3  1999/10/12 14:24:28  AJW
	Added Config_ScrollDistance

	Revision 1.2  1999/10/12 14:11:39  AJW
	Added Config_GraphicsStyle, Config_Snap, Config_SnapDistance, Config_ScrollSpeed

	Revision 1.1  1999/09/27 15:32:22  AJW
	Initial revision


*/

#include "Desk.Core.h"

static char graphicsstyle[256]="Default";
static Desk_bool snap=Desk_TRUE;
static int snapdistance=30;
static int scrollspeed=1;
static int scrolldistance=40;

char *Config_GraphicsStyle(void)
{
	return graphicsstyle;
}

Desk_bool Config_Snap(void)
{
	return snap;
}

int Config_SnapDistance(void)
{
	return snapdistance;
}

int Config_ScrollSpeed(void)
{
	return scrollspeed;
}

int Config_ScrollDistance(void)
{
	return scrolldistance;
}

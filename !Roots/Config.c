/*
	FT - Configuration
	� Alex Waugh 1999

	$Id: Config.c,v 1.5 2000/02/24 19:01:00 uid1 Exp $

*/

#include "Desk.Core.h"

static char graphicsstyle[256]="Default";
static Desk_bool loadgraphicsstyle=Desk_FALSE,importgraphicsstyle=Desk_FALSE;
static Desk_bool snap=Desk_TRUE;
static int snapdistance=30;
static int scrollspeed=1;
static int scrolldistance=40;
static Desk_bool autoincreasesize=Desk_TRUE,autoincreasealways=Desk_FALSE;

char *Config_GraphicsStyle(void)
{
	return graphicsstyle;
}

Desk_bool Config_LoadGraphicsStyle(void)
{
	return loadgraphicsstyle;
}

Desk_bool Config_ImportGraphicsStyle(void)
{
	return importgraphicsstyle;
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

Desk_bool Config_AutoIncreaseSize(void)
{
	return autoincreasesize;
}

Desk_bool Config_AutoIncreaseAlways(void)
{
	return autoincreasealways;
}


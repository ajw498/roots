#ifndef CONFIG_H
#define CONFIG_H

#include "LayoutStruct.h"

extern char choicesread[256],choiceswrite[256];

#define GRAPHICSDIR "Graphics"
#define CONFIGFILE "Config"
#define DEFAULTS "<Roots$Dir>.Defaults"

char *Config_GraphicsStyle(void);

void Config_SaveFileConfig(void);
void Config_LoadFileConfig(void);
void Config_SetJoinMarriages(Desk_bool join);
Desk_bool Config_JoinMarriages(void);
Desk_bool Config_SeparateMarriages(void);
void Config_SetSeparateMarriages(layout *layout,Desk_bool separate);
Desk_bool Config_ImportGraphicsStyle(void);
Desk_bool Config_Title(void);
Desk_bool Config_Snap(void);
Desk_bool Config_FontBlend(void);
int Config_SnapDistance(void);
int Config_ScrollSpeed(void);
int Config_ScrollDistance(void);
Desk_bool Config_AutoIncreaseSize(void);
Desk_bool Config_AutoIncreaseAlways(void);
char *Config_UserDesc(int num);
void Config_Init(void);
void Config_Open(void);

#endif

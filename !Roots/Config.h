#ifndef CONFIG_H
#define CONFIG_H

#define GRAPHICSREAD "Roots:Graphics"
#define GRAPHICSWRITE "<Roots$Write>.Graphics"
#define CONFIGDIR "<Roots$Write>"
#define CONFIGREAD "Roots:Config"
#define CONFIGWRITE "<Roots$Write>.Config"


char *Config_GraphicsStyle(void);

Desk_bool Config_ImportGraphicsStyle(void);

Desk_bool Config_Title(void);

Desk_bool Config_Snap(void);

int Config_SnapDistance(void);

int Config_ScrollSpeed(void);

int Config_ScrollDistance(void);

Desk_bool Config_AutoIncreaseSize(void);

Desk_bool Config_AutoIncreaseAlways(void);

char *Config_UserDesc(int num);

void Config_Init(void);

void Config_Open(void);

#endif

#ifndef CONFIG_H
#define CONFIG_H

char *Config_GraphicsStyle(void);

Desk_bool Config_SaveGraphicsStyle(void);

Desk_bool Config_LoadGraphicsStyle(void);

Desk_bool Config_ImportGraphicsStyle(void);

Desk_bool Config_Snap(void);

int Config_SnapDistance(void);

int Config_ScrollSpeed(void);

int Config_ScrollDistance(void);

Desk_bool Config_AutoIncreaseSize(void);

Desk_bool Config_AutoIncreaseAlways(void);

char *Config_UserDesc(int num);

#endif

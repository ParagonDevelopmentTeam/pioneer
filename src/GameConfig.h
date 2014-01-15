// Copyright © 2013-14 Meteoric Games Ltd
// Licensed under the terms of the GPL v3. See licenses/GPL-3.txt

#ifndef _GAMECONFIG_H
#define _GAMECONFIG_H

#include "IniConfig.h"

class GameConfig : public IniConfig {
public:
	GameConfig();

	void Load();
	bool Save();
};

#endif

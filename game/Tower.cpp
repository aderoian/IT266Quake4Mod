#include "../idlib/precompiled.h"
#pragma hdrstop

#include "Tower.h"
#include "Game_local.h"

TowerManager::TowerManager(void)
{
	gameLocal.Printf("Tower manager created...\n");
}

TowerManager::~TowerManager(void)
{
}

void TowerManager::Init(void)
{
	gameLocal.Printf("Tower manager initialized...\n");
}

void TowerManager::Update(void)
{
}

Tower::Tower(idPlayer* owner, idStr tower)
{
	this->owner = owner;
	this->tower = tower;
}

Tower::~Tower(void)
{
}

void Tower::Init(idVec3 origin)
{
	this->origin = origin;
}

void Tower::SpawnTower()
{
	float yaw;
	idDict dict;

	yaw = owner->viewAngles.yaw;

	dict.Set("classname", "player_animatedentity");
	dict.Set("angle", va("%f", yaw + 180));

	dict.Set("origin", origin.ToString());
	dict.Set("model", "weapon_rocketlauncher_world");

	idEntity* newEnt = NULL;
	gameLocal.SpawnEntityDef(dict, &newEnt);

	if (newEnt) {
		gameLocal.Printf("spawned tower '%s'\n", newEnt->name.c_str());
	}
}

void Tower::Update(void)
{
}

idVec3 Tower::GetOrigin(void)
{
	return origin;
}

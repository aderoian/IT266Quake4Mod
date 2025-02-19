#include "../idlib/precompiled.h"
#pragma hdrstop

#include "Tower.h"
#include "Game_local.h"

Tower::Tower(idPlayer* owner, idStr tower)  
{  
   this->owner = owner;  
   this->tower = tower;  
   init = false;  
   towerEntity = nullptr;  

   gameLocal.towerManager->AddTower(this);  
}

Tower::~Tower(void)
{
	gameLocal.Printf("Tower destroyed...\n");

	if (towerEntity)
		delete towerEntity;

	towerEntity = nullptr;
	init = false;
}

void Tower::Init(idVec3 origin)
{
	this->origin = origin;
	SpawnTower();

	init = true;
}

void Tower::SpawnTower()
{
	if (towerEntity) return;

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
	else {
		gameLocal.Printf("failed to spawn tower\n");
		return;
	}

	towerEntity = newEnt;
	init = true;
}

void Tower::Update(void)
{
	if (!init || !towerEntity) return;

	if (gameLocal.GetTime() % shootDelay == 0)
		Shoot();
}

idVec3 Tower::GetOrigin(void)
{
	return origin;
}

bool Tower::CanShoot(void)
{
	return gameLocal.towerManager->CanTowersShoot();
}

void Tower::Shoot(void)
{
	if (!CanShoot()) return;
	ForceShoot();
}

void Tower::ForceShoot(void)
{
	// TODO: shoot logic
}

TowerManager::TowerManager(void)
{
	gameLocal.Printf("Tower manager created...\n");

	buildMode = false;
	buildTower = "tower";

	lastWaveStart = -1;
	lastWaveEnd = -1;
	waveDelay = 1 * 60 * 10 ^ 3;
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
	// Update Wave
	if (!wave && gameLocal.GetTime() > (lastWaveEnd + waveDelay)) {
		wave = new Wave();
		wave->Init();
	}

	// Update Towers
	for (int i = 0; i < towers.Num(); i++)
	{
		towers[i]->Update();
	}
}

void TowerManager::AddTower(Tower* tower)
{
	towers.Append(tower);
}

bool TowerManager::CanTowersShoot(void)
{
	if (wave) {
		return wave->HasStarted() && !wave->HasEnded();
	}

	return false;
}

void TowerManager::ToggleBuild(void)
{
	buildMode = !buildMode;
}

void TowerManager::BuildTower(idVec3 origin)
{
	if (buildTower == "tower") return;

	Tower* tower = new Tower(gameLocal.GetLocalPlayer(), buildTower);
	tower->Init(origin);
}

Wave::Wave(void)
{
}

Wave::~Wave(void)
{
}

void Wave::Init(void)
{
}

void Wave::Update(void)
{
}

bool Wave::HasStarted(void)
{
	return false;
}

bool Wave::HasEnded(void)
{
	return false;
}

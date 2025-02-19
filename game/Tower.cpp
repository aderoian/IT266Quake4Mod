#include "../idlib/precompiled.h"
#pragma hdrstop

#include "Tower.h"
#include "Game_local.h"

Tower::Tower(idPlayer* owner, const TowerDef* tower)  
{  
   this->owner = owner;  
   this->tower = tower;  
   init = false;  
   towerEntity = nullptr;  

   id = gameLocal.towerManager->AddTower(this);  
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

	if (!tower) {
		gameLocal.Printf("failed to spawn tower: no tower definition\n");
		return;
	}
	
	dict.Set("classname", "player_animatedentity");
	dict.Set("angle", va("%f", yaw + 180));

	dict.Set("origin", origin.ToString());
	dict.Set("model", tower->model.c_str());

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

	towerId = 0;

	buildMode = false;
	buildTower = nullptr;

	lastWaveStart = -1;
	lastWaveEnd = -1;
	waveDelay = 1 * 60 * 10 ^ 3;

	towerDefinitions = TowerDefList();
	towers = idList<Tower*>();

	// Register Tower Definitions
	Register(new TowerDef("dark_matter", "weapon_dmg_world", ResourceCost(), 0, 0, 0));
	Register(new TowerDef("gauntlet", "weapon_gauntlet_world", ResourceCost(), 0, 0, 0));
	Register(new TowerDef("grenade_launcher", "weapon_grenadelauncher_world", ResourceCost(), 0, 0, 0));
	Register(new TowerDef("hyperblaster", "weapon_hyperblaster_world", ResourceCost(), 0, 0, 0));
	Register(new TowerDef("lightning", "weapon_lightninggun_world", ResourceCost(), 0, 0, 0));
	Register(new TowerDef("machine_gun", "weapon_machinegun_world", ResourceCost(), 0, 0, 0));
	Register(new TowerDef("nailgun", "weapon_nailgun_world", ResourceCost(), 0, 0, 0));
	Register(new TowerDef("napalm", "weapon_napalmgun_world", ResourceCost(), 0, 0, 0));
	Register(new TowerDef("railgun", "weapon_railgun_world", ResourceCost(), 0, 0, 0));
	Register(new TowerDef("rocketlauncher", "weapon_rocketlauncher_world", ResourceCost(), 0, 0, 0));
}

TowerManager::~TowerManager(void)
{
	gameLocal.Printf("Tower manager destroyed...\n");
	for (int i = 0; i < towers.Num(); i++)
	{
		delete towers[i];
	}
	towers.Clear();
	delete wave;

	delete& towerDefinitions;
}

void TowerManager::Init(void)
{
	gameLocal.Printf("Tower manager initialized...\n");
}

void TowerManager::Register(TowerDef* def)
{
	towerDefinitions.AddDef(def);
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

int TowerManager::AddTower(Tower* tower)
{
	towers.Append(tower);
	return towerId++;
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
	if (!buildTower) return;

	gameLocal.Printf("Building tower: %s\n", buildTower->name.c_str());
	Tower* tower = new Tower(gameLocal.GetLocalPlayer(), buildTower);
	tower->Init(origin);
}

void TowerManager::ArgCompletion_TowerDefs(const idCmdArgs& args, void(*callback)(const char* s)) {
	int i;

	auto towerManager = gameLocal.towerManager;
	for (i = 0; i < towerManager->towerDefinitions.Num(); i++) {
		if (towerManager->towerDefinitions[i]) {
			callback(va("%s %s", args.Argv(0), towerManager->towerDefinitions[i]->name.c_str()));
		}
	}
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

TowerDefList::TowerDefList(void)
{
	towerDefs = idList<TowerDef*>();
	keyMap = idDict();
}

TowerDefList::~TowerDefList(void)
{
	for (int i = 0; i < towerDefs.Num(); i++)
	{
		delete towerDefs[i];
	}

	towerDefs.Clear();
	keyMap.Clear();
}

void TowerDefList::AddDef(TowerDef* def)
{
	int index = towerDefs.Append(def);
	keyMap.SetInt(def->name, index);
}

int TowerDefList::Num()
{
	return towerDefs.Num();
}

TowerDef* TowerDefList::GetDef(const char* name)
{
	int index = keyMap.GetInt(name, "-1");
	if (index == -1) return nullptr;

	return towerDefs[index];
}

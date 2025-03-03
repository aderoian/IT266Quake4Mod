#include "../idlib/precompiled.h"
#pragma hdrstop

#include "Tower.h"
#include "Game_local.h"
#include "Projectile.h"

Tower::Tower(idPlayer* owner, TowerDef* tower, idVec3* origin)  
{  
   this->owner = owner;  
   this->towerDef = tower;  
   init = false;  
   towerEntity = nullptr;

   this->origin = origin;
   id = gameLocal.towerManager->entityId++;

   auto tmpName = va("tower_%s_%d", towerDef->name.c_str(), id);
   name = new char[strlen(tmpName) + 1];
   strcpy(name, tmpName);

   level = 1;
   lastShot = -1;

   SpawnTower();
}

Tower::~Tower(void)
{
	gameLocal.Printf("Tower destroyed...\n");
	owner = nullptr;

	if (origin) 
		delete origin;

	delete[] name;
	delete towerEntity;
	towerEntity = nullptr;
	init = false;

}

void Tower::SpawnTower()
{
	if (towerEntity) return;

	float yaw;
	idDict dict;

	yaw = owner->viewAngles.yaw;

	if (!towerDef) {
		gameLocal.Printf("failed to spawn tower: no tower definition\n");
		return;
	}
	
	dict.Set("classname", "player_animatedentity");
	dict.Set("name", name);
	dict.Set("angle", va("%f", yaw + 180));

	dict.Set("origin", origin->ToString());
	dict.Set("model", towerDef->model.c_str());

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
	towerEntity->health = 100; // TODO: tower health
	init = true;
}

void Tower::ShootHitscan(Tower* tower, idVec3 target, const idDict* dict)
{
	idVec3 start = *tower->origin + (idVec3(0, 0, 1) * 50);
	idVec3 dir = target - start;
	dir.Normalize();
	gameLocal.HitScan(*dict, start, dir, start, tower->towerEntity);
}

void Tower::Update(void)
{
	if (!init || !towerEntity) return;

	if (towerEntity->health <= 0) {
		gameLocal.Printf("Tower %s has been destroyed\n", name);
		gameLocal.towerManager->DestroyTower(this);
		return;
	}
	
	if (gameLocal.GetTime() - lastShot > towerDef->shootDelay)
		Shoot();
}

int Tower::GetDamage(void)
{
	return towerDef->damage;
}

int Tower::GetRange(void)
{
	return towerDef->range;
}

bool Tower::CanShoot(void)
{
	return gameLocal.towerManager->CanTowersShoot();
}

void Tower::Shoot(void)
{
	lastShot = gameLocal.GetTime();

	if (!CanShoot()) return;
	ForceShoot();
}

void Tower::ForceShoot(void)
{
	TowerShootFunc_t shootFunc = towerDef->shootFunc;
	if (shootFunc) {
		idVec3 target = gameLocal.towerManager->wave->GetNearestMonster(this);
		gameLocal.Printf("Shooting at %s\n", target.ToString());
		shootFunc(this, target);
	}
}

void Tower::Upgrade(void)
{
	if (level > towerDef->upgrades.Num()) return;

	TowerDef upgrade = towerDef->upgrades[level-1];
}

void Tower::ShootDarkMatter(Tower* tower, idVec3 target)
{

}

void Tower::ShootGauntlet(Tower* tower, idVec3 target)
{
}

void Tower::ShootGrenadeLauncher(Tower* tower, idVec3 target)
{
}

void Tower::ShootHyperBlaster(Tower* tower, idVec3 target)
{
}

void Tower::ShootLightning(Tower* tower, idVec3 target)
{
	const idDict* dict = gameLocal.FindEntityDefDict("hitscan_lightninggun", false);
	ShootHitscan(tower, target, dict); 
}

void Tower::ShootMachineGun(Tower* tower, idVec3 target)
{
	
	const idDict *dict = gameLocal.FindEntityDefDict("hitscan_bullet", false);
	ShootHitscan(tower, target, dict);
}

void Tower::ShootNailGun(Tower* tower, idVec3 target)
{
}

void Tower::ShootNapalm(Tower* tower, idVec3 target)
{
}

void Tower::ShootRailgun(Tower* tower, idVec3 target)
{
	const idDict* dict = gameLocal.FindEntityDefDict("hitscan_railgun", false);
	ShootHitscan(tower, target, dict);
}

void Tower::ShootRocketLauncher(Tower* tower, idVec3 target)
{
}

void Tower::GenerateGold(Tower* tower, idVec3 target)
{
	gameLocal.Printf("Generating gold...\n");
	tower->owner->inventory.gold += tower->GetDamage();
}

void Tower::GenerateEnergy(Tower* tower, idVec3 target)
{
	tower->owner->inventory.energy += tower->GetDamage();
}

void Tower::GenerateStone(Tower* tower, idVec3 target)
{
	tower->owner->inventory.stone += tower->GetDamage();
}

void Tower::GenerateWood(Tower* tower, idVec3 target)
{
	tower->owner->inventory.wood += tower->GetDamage();
}

void Tower::GenerateBuilder(Tower* tower, idVec3 target)
{
	tower->owner->inventory.builder += tower->GetDamage();
}

TowerManager::TowerManager(void)
{
	gameLocal.Printf("Tower manager created...\n");

	entityId = 0;

	buildMode = false;
	buildTower = nullptr;

	lastWaveStart = -1;
	lastWaveEnd = -1;
	waveDelay = 1 * 60 * 10 ^ 3;
	waveCount = 0;
	wave = nullptr;

	towerDefinitions = DefList<TowerDef*>();
	monsterDefinitions = DefList<WaveMonsterDef*>();
	towers = idList<Tower*>();

	center = new idVec3(0, 0, 0);

	// Register Tower Definitions
	RegisterTower(new TowerDef("dark_matter", "weapon_dmg_world", ResourceCost(), 0, 0, 0, Tower::ShootDarkMatter, {}));
	RegisterTower(new TowerDef("gauntlet", "weapon_gauntlet_world", ResourceCost(), 0, 0, 0, Tower::ShootGauntlet, {}));
	RegisterTower(new TowerDef("grenade_launcher", "weapon_grenadelauncher_world", ResourceCost(), 0, 0, 0, Tower::ShootGrenadeLauncher, {}));
	RegisterTower(new TowerDef("hyperblaster", "weapon_hyperblaster_world", ResourceCost(), 0, 0, 0, Tower::ShootHyperBlaster, {}));
	RegisterTower(new TowerDef("lightning", "weapon_lightninggun_world", ResourceCost(), 0, 500, 0, Tower::ShootLightning, {}));
	RegisterTower(new TowerDef("machine_gun", "weapon_machinegun_world", ResourceCost(), 10, 500, 250, Tower::ShootMachineGun, {}));
	RegisterTower(new TowerDef("nailgun", "weapon_nailgun_world", ResourceCost(), 0, 0, 0, Tower::ShootNailGun, {}));
	RegisterTower(new TowerDef("napalm", "weapon_napalmgun_world", ResourceCost(), 0, 0, 0, Tower::ShootNapalm, {}));
	RegisterTower(new TowerDef("railgun", "weapon_railgun_world", ResourceCost(), 0, 500, 0, Tower::ShootRailgun, {}));
	RegisterTower(new TowerDef("rocketlauncher", "weapon_rocketlauncher_world", ResourceCost(), 0, 0, 0, Tower::ShootRocketLauncher, {}));

	// Economy Towers
	RegisterTower(new TowerDef("gold_generator", "models/pick_ups/sp_pickups/sp_darkmatter.lwo", ResourceCost(0, 0, 0), 5, 0, 1000, Tower::GenerateGold, {}));
	RegisterTower(new TowerDef("energy_generator", "models/pick_ups/sp_pickups/sp_darkmatter.lwo", ResourceCost(0, 0, 0), 5, 0, 1000, Tower::GenerateEnergy, {}));
	RegisterTower(new TowerDef("stone_generator", "models/pick_ups/sp_pickups/sp_darkmatter.lwo", ResourceCost(0, 0, 0), 5, 0, 1000, Tower::GenerateStone, {}));
	RegisterTower(new TowerDef("wood_generator", "models/pick_ups/sp_pickups/sp_darkmatter.lwo", ResourceCost(0, 0, 0), 5, 0, 1000, Tower::GenerateWood, {}));
	RegisterTower(new TowerDef("builder_generator", "models/pick_ups/sp_pickups/sp_darkmatter.lwo", ResourceCost(0, 0, 0), 5, 0, 1000, Tower::GenerateBuilder, {}));

	// Register Monster Definitions
	RegisterMonster(new WaveMonsterDef("monster_berserker", 100, 50));
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

	delete center;
	delete& towerDefinitions;
}

void TowerManager::Init(void)
{
	gameLocal.Printf("Tower manager initialized...\n");
}

void TowerManager::RegisterTower(TowerDef* def)
{
	towerDefinitions.AddDef(def);
}

void TowerManager::RegisterMonster(WaveMonsterDef* def)
{
	monsterDefinitions.AddDef(def);
}

void TowerManager::Update(void)
{
	// Update Wave
	if (!wave && gameLocal.GetTime() > (lastWaveEnd + waveDelay)) {
		/*wave = new Wave();
		wave->Init();*/
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
	CalculateCenter();
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

	idVec3* originPtr = new idVec3(origin);
	gameLocal.Printf("Building tower '%s' at: '%s'\n", buildTower->name.c_str(), origin.ToString());
	Tower* tower = new Tower(gameLocal.GetLocalPlayer(), buildTower, originPtr);
	AddTower(tower);
}

void TowerManager::DestroyTower(Tower* tower)
{
	if (!tower) return;
	towers.Remove(tower);
	delete tower;
	CalculateCenter();
}

void TowerManager::CalculateCenter(void)
{
	int x = 0, y = 0, z = 0;
	for (int i = 0; i < towers.Num(); i++)
	{
		idVec3 origin = * towers[i]->origin;
		x += origin.x;
		y += origin.y;
		z += origin.z;

		gameLocal.Printf("%d %d %d", x, y, z);
	}

	delete center;
	center = new idVec3(x / towers.Num(), y / towers.Num(), z / towers.Num());
	gameLocal.Printf("Center: %s\n", center->ToString());
}

void TowerManager::SetWave(Wave* wave)
{
	this->wave = wave; 
}

Tower* TowerManager::FindTower(int id)
{
	for (int i = 0; i < towers.Num(); i++)
	{
		if (towers[i]->id == id) {
			return towers[i];
		}
	}
	return nullptr;
}

Tower* TowerManager::FindTower(idVec3 origin)
{
	Tower* closestTower = nullptr;
	float closestDistance = 999999999;
	for (int i = 0; i < towers.Num(); i++)
	{
		float distance = origin.Dist(*towers[i]->origin);
		if (distance < closestDistance) {
			closestTower = towers[i];
			closestDistance = distance;
		}
	}
	return closestTower;
}

Tower* TowerManager::FindTower(const char* name)
{
	gameLocal.Printf("Finding tower '%s'\n", name);
	for (int i = 0; i < towers.Num(); i++)
	{
		auto tName = towers[i]->name;
		gameLocal.Printf("Tower: %s\n", tName);
		gameLocal.Printf("Comparing: '%s' and '%s'\n", name, tName);
		if (strcmp(tName, name) == 0) {
			gameLocal.Printf("Found tower '%s'\n", name);
			return towers[i];
		}
	}
	return nullptr;
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

Wave::Wave(int startingMonsters, idList<idStr> monsterTypes)
{
	this->startingMonsters = startingMonsters;
	this->monstersLeft = startingMonsters;
	this->monsterTypes = monsterTypes;
	this->monsters = idList<idAI*>();
}

Wave::~Wave(void)
{
}

void Wave::Init(void)
{
	for (int i = 0; i < startingMonsters; i++)
	{
		SpawnMonster(monsterTypes[gameLocal.random.RandomInt(monsterTypes.Num())], * gameLocal.towerManager->center);
	}
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

bool Wave::IsMonsterMember(idAI* monster)
{
	if (!monster) return false;
	return monsters.FindIndex(monster) != -1;
}

void Wave::OnMonsterKilled(idAI* monster)
{
	if (!IsMonsterMember(monster)) return;

	monsters.Remove(monster);
	monstersLeft--;
	if (monstersLeft == 0) {
		// End Wave
	}
}

void Wave::OnAttack(idAI* monster, idEntity* target, idEntity* projectile)
{
	if (!monster || !target) return;
	if (!IsMonsterMember(monster)) return;

	gameLocal.Printf("Monster '%s' attacked target '%s'\n", monster->name.c_str(), target->name.c_str());

	idList<idStr> name;
	monster->name.Split(name, '-');
	if (name.Num() < 2) return;

	gameLocal.Printf("Monster '%s' attacked target '%s'\n", name[0].c_str(), target->name.c_str());
	int damage = gameLocal.towerManager->monsterDefinitions[name[0]]->baseDamage;
	auto tower = gameLocal.towerManager->FindTower(target->name);

	if (tower) {
		gameLocal.Printf("Monster '%s' attacked target '%s' with damage '%d'\n", name[0].c_str(), target->name.c_str(), damage);
		tower->towerEntity->health -= damage;
	}
}

idVec3 Wave::GetNearestMonster(Tower* tower)
{
	int dist = tower->towerDef->GetRange(tower->level);
	idVec3 loc = idVec3(0, 0, 0);
	if (monsters.Num() == 0) return loc;

	idAI* monster = nullptr;
	for (int i = 0; i < monsters.Num(); i++)
	{
		monster = monsters[i];
		if (!monster) continue;

		if (dist == -1) {
			loc = monster->GetPhysics()->GetOrigin();
		}
		else if (monster->GetPhysics()->GetOrigin().Dist(*tower->origin) < dist) {
			loc = monster->GetPhysics()->GetOrigin();
		}
	}
	return loc;
}

void Wave::SpawnMonster(idStr type, idVec3 origin)
{
	WaveMonsterDef* monsterDef = gameLocal.towerManager->monsterDefinitions[type];
	if (!monsterDef) return;

	idDict dict;
	dict.Set("classname", type);
	dict.Set("origin", origin.ToString());
	dict.Set("name", va("%s-%d", monsterDef->name.c_str(), gameLocal.towerManager->entityId++));
	dict.Set("target", gameLocal.towerManager->towers[0]->name);

	idEntity* newEnt = nullptr;
	gameLocal.SpawnEntityDef(dict, &newEnt);

	if (newEnt) {
		gameLocal.Printf("spawned entity '%s'\n", newEnt->name.c_str());
	}

	idAI* ai = dynamic_cast<idAI*>(newEnt);
	if (ai) {
		ai->SetEnemy(gameLocal.towerManager->towers[0]->towerEntity);
	}

	monsters.Append(ai);
}

template class DefList<TowerDef*>;
template class DefList<WaveMonsterDef*>;

template <class T>
DefList<T>::DefList(void)
{
	defs = idList<T>();
	keyMap = idDict();
}

template <class T>
DefList<T>::~DefList(void)
{
	for (int i = 0; i < defs.Num(); i++)
	{
		delete defs[i];
	}

	defs.Clear();
	keyMap.Clear();
}

template <class T>
void DefList<T>::AddDef(T def)
{
	int index = defs.Append(def);
	keyMap.SetInt(def->name, index);
}

template <class T>
int DefList<T>::Num()
{
	return defs.Num();
}

template <class T>
T DefList<T>::GetDef(const char* name)
{
	int index = keyMap.GetInt(name, "-1");
	if (index == -1) return nullptr;

	return defs[index];
}

int TowerDef::GetDamage(int level)
{
	if (level == 1) return damage;
	if (level > upgrades.Num()) return damage;

	return upgrades[level - 1].damage;
}

int TowerDef::GetRange(int level)
{
	if (level == 1) return range;
	if (level > upgrades.Num()) return range;
	return upgrades[level - 1].range;
}

int TowerDef::GetShootDelay(int level)
{
	if (level == 1) return shootDelay;
	if (level > upgrades.Num()) return shootDelay;
	return upgrades[level - 1].shootDelay;
}

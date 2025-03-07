#include "../idlib/precompiled.h"
#pragma hdrstop

#include "Tower.h"
#include "Game_local.h"
#include "Projectile.h"
#include "Player.h"

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
	idEntity* hit = gameLocal.HitScan(*dict, start, dir, start, tower->towerEntity, false);
}

void Tower::Update(void)
{
	if (!init || !towerEntity) return;

	if (towerEntity->health <= 0) {
		gameLocal.Printf("Tower %s has been destroyed\n", name);
		gameLocal.towerManager->DestroyTower(this);
		return;
	}
	
	if (gameLocal.GetTime() - lastShot > towerDef->GetShootDelay(level))
		Shoot();
}

int Tower::GetDamage(void)
{
	return towerDef->GetDamage(level);
}

int Tower::GetRange(void)
{
	return towerDef->GetRange(level);
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
		shootFunc(this, target);
	}
}

void Tower::Upgrade(void)
{
	if (level > towerDef->upgrades.Num()) return;

	idPlayer* player = gameLocal.GetLocalPlayer();
	if (!player) return;

	TowerDef upgrade = towerDef->upgrades[level - 1];
	if (!player->inventory.ProcessTransaction(upgrade.cost, false) || !player->inventory.ProcessBuilderTransaction(1, false)) {
		gameLocal.Printf("Unable to upgrade %s to level %d, insufficient resources.", towerDef->name.c_str(), level + 1);
		return;
	}

	player->inventory.ProcessTransaction(upgrade.cost, true);
	player->inventory.ProcessBuilderTransaction(1, true);

	level++;
}

void Tower::ShootDarkMatter(Tower* tower, idVec3 target)
{
	const idDict* dict = gameLocal.FindEntityDefDict("projectile_dmg", false);

	idVec3 start = *tower->origin + (idVec3(0, 0, 1) * 50);
	idVec3 dir = target - start;
	dir.Normalize();

	idEntity* ent;
	gameLocal.SpawnEntityDef(*dict, &ent);
	if (!ent) {
		gameLocal.Printf("ERR: Failed to spawn dmg projectile\n");
		return;
	}

	idProjectile* proj = static_cast<idProjectile*>(ent);
	proj->Create(tower->towerEntity, start, dir, tower->towerEntity);
	proj->Launch(start, dir, dir, 0, 1);
}

void Tower::ShootGauntlet(Tower* tower, idVec3 target)
{
	const idDict* dict = gameLocal.FindEntityDefDict("projectile_hyperblaster", false);

	idList<idVec3> dirs;
	dirs.Append(idVec3(1, 0, 0));
	dirs.Append(idVec3(-1, 0, 0));
	dirs.Append(idVec3(0, 1, 0));
	dirs.Append(idVec3(0, -1, 0));
	dirs.Append(idVec3(0.5, 0.5, 0));
	dirs.Append(idVec3(-0.5, 0.5, 0));
	dirs.Append(idVec3(0.5, -0.5, 0));
	dirs.Append(idVec3(-0.5, -0.5, 0));

	idVec3 start = *tower->origin + (idVec3(0, 0, 1) * 50);
	for (int i = 0; i < dirs.Num(); i++) {
		idVec3 dir = dirs[i];

		idEntity* ent;
		gameLocal.SpawnEntityDef(*dict, &ent);
		if (!ent) {
			gameLocal.Printf("ERR: Failed to spawn gauntlet projectile\n");
			return;
		}

		idProjectile* proj = static_cast<idProjectile*>(ent);
		proj->Create(tower->towerEntity, start, dir, tower->towerEntity);
		proj->Launch(start, dir, idVec3(0, 0, 0), 0, 1);
	}
}

void Tower::ShootGrenadeLauncher(Tower* tower, idVec3 target)
{
	const idDict* dict = gameLocal.FindEntityDefDict("projectile_grenade", false);
	idVec3 start = *tower->origin + idVec3(0, 0, 50);
	idVec3 flatDir = target - start;
	flatDir.z = 0;

	float range = flatDir.Length();
	float velocity = 700.0f;
	float gravity = fabs(gameLocal.GetGravity().z);

	float tmp = (gravity * range) / (velocity * velocity);
	if (tmp > 1) {
		gameLocal.Printf("ERR: Failed to calculate grenade angle\n");
		return;
	}
	float angle = 0.5f * asinf(tmp);

	flatDir.Normalize();
	idVec3 dir = flatDir * cos(angle) + idVec3(0, 0, sin(angle));
	dir.Normalize();

	idEntity* ent;
	gameLocal.SpawnEntityDef(*dict, &ent);
	if (!ent) {
		gameLocal.Printf("ERR: Failed to spawn grenade projectile\n");
		return;
	}

	idProjectile* proj = static_cast<idProjectile*>(ent);
	proj->Create(tower->towerEntity, start, dir, tower->towerEntity);
	proj->Launch(start, dir, idVec3(0, 0, 0), 0, 1);
}

void Tower::ShootHyperBlaster(Tower* tower, idVec3 target)
{
	const idDict* dict = gameLocal.FindEntityDefDict("projectile_hyperblaster", false);

	if (!dict) {
		gameLocal.Printf("ERR: Failed to find hyperblaster projectile\n");
		return;
	}

	idVec3 start = *tower->origin + (idVec3(0, 0, 1) * 50);
	idVec3 dir = target - start;
	dir.Normalize();

	idEntity* ent;
	gameLocal.SpawnEntityDef(*dict, &ent);
	if (!ent) {
		gameLocal.Printf("ERR: Failed to spawn hyperblaster projectile\n");
		return;
	}

	idProjectile* proj = static_cast<idProjectile*>(ent);
	proj->Create(tower->towerEntity, start, dir, tower->towerEntity);
	proj->Launch(start, dir, idVec3(0,0,0), 0, 1);
}

void Tower::ShootLightning(Tower* tower, idVec3 target)
{
	ShootHitscan(tower, target, gameLocal.FindEntityDefDict("hitscan_lightninggun", false));
}

void Tower::ShootMachineGun(Tower* tower, idVec3 target)
{
	
	const idDict *dict = gameLocal.FindEntityDefDict("hitscan_bullet", false);
	ShootHitscan(tower, target, dict);
}

void Tower::ShootNailGun(Tower* tower, idVec3 target)
{
	const idDict* dict = gameLocal.FindEntityDefDict("projectile_nail", false);

	idVec3 start = *tower->origin + (idVec3(0, 0, 1) * 50);
	idVec3 dir = target - start;
	dir.Normalize();

	idEntity* ent;
	gameLocal.SpawnEntityDef(*dict, &ent);
	if (!ent) {
		gameLocal.Printf("ERR: Failed to spawn nail projectile\n");
		return;
	}

	idProjectile* proj = static_cast<idProjectile*>(ent);
	proj->Create(tower->towerEntity, start, dir, tower->towerEntity);
	proj->Launch(start, dir, dir, 0, 1);
}

void Tower::ShootNapalm(Tower* tower, idVec3 target)
{
	const idDict* dict = gameLocal.FindEntityDefDict("projectile_napalm", false);
	idVec3 start = *tower->origin + idVec3(0, 0, 50);
	idVec3 flatDir = target - start;
	flatDir.z = 0;

	float range = flatDir.Length();
	float velocity = 1200.0f;
	float gravity = fabs(gameLocal.GetGravity().z);

	float tmp = (gravity * range) / (velocity * velocity);
	if (tmp > 1) {
		gameLocal.Printf("ERR: Failed to calculate grenade angle\n");
		return;
	}
	float angle = 0.5f * asinf(tmp);

	flatDir.Normalize();
	idVec3 dir = flatDir * cos(angle) + idVec3(0, 0, sin(angle));
	dir.Normalize();

	idEntity* ent;
	gameLocal.SpawnEntityDef(*dict, &ent);
	if (!ent) {
		gameLocal.Printf("ERR: Failed to spawn napalm projectile\n");
		return;
	}

	idProjectile* proj = static_cast<idProjectile*>(ent);
	proj->Create(tower->towerEntity, start, dir, tower->towerEntity);
	proj->Launch(start, dir, idVec3(0, 0, 0), 0, 1);
}

void Tower::ShootRailgun(Tower* tower, idVec3 target)
{
	const idDict* dict = gameLocal.FindEntityDefDict("hitscan_railgun", false);
	ShootHitscan(tower, target, dict);
}

void Tower::ShootRocketLauncher(Tower* tower, idVec3 target)
{
	const idDict* dict = gameLocal.FindEntityDefDict("projectile_rocket", false);
	
	idVec3 start = *tower->origin + (idVec3(0, 0, 1) * 50);
	idVec3 dir = target - start;
	dir.Normalize();

	idEntity* ent;
	gameLocal.SpawnEntityDef(*dict, &ent);
	if (!ent) {
		gameLocal.Printf("ERR: Failed to spawn rocket projectile\n");
		return;
	}

	idProjectile* proj = static_cast<idProjectile*>(ent);
	proj->Create(tower->towerEntity, start, dir, tower->towerEntity);
	proj->Launch(start, dir, dir, 0, 1);
}

void Tower::GenerateGold(Tower* tower, idVec3 target)
{
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
	waveDelay = 15;
	waveCount = 0;
	wave = nullptr;

	towerDefinitions = DefList<TowerDef*>();
	monsterDefinitions = DefList<WaveMonsterDef*>();
	towers = idList<Tower*>();
	gameStarted = false;

	// Register Tower Definitions
	RegisterTower(new TowerDef("dark_matter", "weapon_dmg_world", ResourceCost(), 10, 10000, 2000, Tower::ShootDarkMatter, {}));
	RegisterTower(new TowerDef("gauntlet", "weapon_gauntlet_world", ResourceCost(), 10, 10000, 250, Tower::ShootGauntlet, {}));
	RegisterTower(new TowerDef("grenade_launcher", "weapon_grenadelauncher_world", ResourceCost(), 10, 10000, 1000, Tower::ShootGrenadeLauncher, {}));
	RegisterTower(new TowerDef("hyperblaster", "weapon_hyperblaster_world", ResourceCost(), 10, 1000, 250, Tower::ShootHyperBlaster, {}));
	RegisterTower(new TowerDef("lightning", "weapon_lightninggun_world", ResourceCost(), 10, 500, 250, Tower::ShootLightning, {}));
	RegisterTower(new TowerDef("machine_gun", "weapon_machinegun_world", ResourceCost(10, 10, 10), 10, 500, 100000, Tower::ShootMachineGun, {}));
	RegisterTower(new TowerDef("nailgun", "weapon_nailgun_world", ResourceCost(), 10, 10000, 250, Tower::ShootNailGun, {}));
	RegisterTower(new TowerDef("napalm", "weapon_napalmgun_world", ResourceCost(), 10, 10000, 250, Tower::ShootNapalm, {}));
	RegisterTower(new TowerDef("railgun", "weapon_railgun_world", ResourceCost(), 10, 500, 250, Tower::ShootRailgun, {}));
	RegisterTower(new TowerDef("rocketlauncher", "weapon_rocketlauncher_world", ResourceCost(), 10, 10000, 1500, Tower::ShootRocketLauncher, {}));

	// Economy Towers
	RegisterTower(new TowerDef("gold_generator", "models/pick_ups/sp_pickups/sp_darkmatter.lwo", ResourceCost(0, 0, 0), 5, 0, 1000, Tower::GenerateGold, {}));
	RegisterTower(new TowerDef("energy_generator", "models/pick_ups/sp_pickups/sp_darkmatter.lwo", ResourceCost(0, 0, 0), 5, 0, 1000, Tower::GenerateEnergy, {}));
	RegisterTower(new TowerDef("stone_generator", "models/pick_ups/sp_pickups/sp_darkmatter.lwo", ResourceCost(0, 0, 0), 5, 0, 1000, Tower::GenerateStone, {}));
	RegisterTower(new TowerDef("wood_generator", "models/pick_ups/sp_pickups/sp_darkmatter.lwo", ResourceCost(0, 0, 0), 5, 0, 1000, Tower::GenerateWood, {}));
	RegisterTower(new TowerDef("builder_generator", "models/pick_ups/sp_pickups/sp_darkmatter.lwo", ResourceCost(0, 0, 0), 5, 0, 1000, Tower::GenerateBuilder, {}));

	// Register Monster Definitions
	RegisterMonster(new WaveMonsterDef("monster_berserker", 100, 50, 1));
	RegisterMonster(new WaveMonsterDef("monster_gladiator", 100, 50, 1));
	RegisterMonster(new WaveMonsterDef("monster_bossbuddy", 100, 50, 1));
	//RegisterMonster(new WaveMonsterDef("monster_convoy_ground", 100, 50, 1));
	//RegisterMonster(new WaveMonsterDef("monster_convoy_hover", 100, 50, 1));
	//RegisterMonster(new WaveMonsterDef("monster_fatty", 100, 50, 1));
	RegisterMonster(new WaveMonsterDef("monster_grunt", 100, 50, 1));
	RegisterMonster(new WaveMonsterDef("monster_gunner", 100, 50, 1));
	RegisterMonster(new WaveMonsterDef("monster_hh_tank", 100, 50, 1));
	RegisterMonster(new WaveMonsterDef("monster_iron_maiden", 100, 50, 1));
	RegisterMonster(new WaveMonsterDef("monster_lt_tank", 100, 50, 1));
	RegisterMonster(new WaveMonsterDef("monster_makron", 100, 50, 1));
	RegisterMonster(new WaveMonsterDef("monster_network_guardian", 100, 50, 1));
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
	if (!gameStarted) {
		if (towers.Num() > 20) {
			gameStarted = true;
		}
		else {
			return;
		}
	}

	if (wave) {
		if (wave->HasEnded()) {
			delete wave;
			wave = nullptr;
			lastWaveEnd = gameLocal.GetTime();
		}
		else {
			wave->Update();
		}
	} else if (gameLocal.GetTime() - lastWaveEnd > waveDelay){
		SpawnWave();
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

void TowerManager::SetBuildTowerFromKey(int impulse)
{
	switch (impulse) {
	case 0:
		buildTower = towerDefinitions["machine_gun"];
		break;
	case 1:
		buildTower = towerDefinitions["gauntlet"];
		break;
	case 2:
		buildTower = towerDefinitions["grenade_launcher"];
		break;
	case 3:
		buildTower = towerDefinitions["rocketlauncher"];
		break;
	case 4:
		buildTower = towerDefinitions["nailgun"];
		break;
	case 5:
		buildTower = towerDefinitions["napalm"];
		break;
	case 6:
		buildTower = towerDefinitions["hyperblaster"];
		break;
	case 7:
		buildTower = towerDefinitions["railgun"];
		break;
	case 8:
		buildTower = towerDefinitions["lightning"];
		break;
	case 9:
		buildTower = towerDefinitions["dark_matter"];
		break;
	case 13:
		buildTower = towerDefinitions["gold_generator"];
		break;
	case 14:
		buildTower = towerDefinitions["energy_generator"];
		break;
	case 15:
		buildTower = towerDefinitions["stone_generator"];
		break;
	case 17:
		buildTower = towerDefinitions["wood_generator"];
		break;
	case 18:
		buildTower = towerDefinitions["builder_generator"];
		break;
	
	}

	gameLocal.Printf("Build tower set to: '%s' from KEY\n", buildTower->name.c_str());
}

void TowerManager::BuildTower(idVec3 origin)
{
	if (!buildTower) return;

	idPlayer* player = gameLocal.GetLocalPlayer();
	if (!player) return;

	if (!player->inventory.ProcessTransaction(buildTower->cost, false) || !player->inventory.ProcessBuilderTransaction(1, false)) {
		gameLocal.Printf("Unable to build tower '%s', insufficient resources.", buildTower->name.c_str());
		return;
	}

	player->inventory.ProcessTransaction(buildTower->cost, true);
	player->inventory.ProcessBuilderTransaction(1, true);

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
}

//void TowerManager::CalculateCenter(void)
//{
//	if (towers.Num() == 0) {
//		delete center;
//		center = new idVec3(0, 0, 0);
//		return;
//	}
//
//	int x = 0, y = 0, z = 0;
//	for (int i = 0; i < towers.Num(); i++)
//	{
//		idVec3 origin = * towers[i]->origin;
//		x += origin.x;
//		y += origin.y;
//		z += origin.z;
//
//		gameLocal.Printf("%d %d %d", x, y, z);
//	}
//
//	delete center;
//	center = new idVec3(x / towers.Num(), y / towers.Num(), z / towers.Num());
//	gameLocal.Printf("Center: %s\n", center->ToString());
//}

void TowerManager::SetWave(Wave* wave)
{
	if (this->wave) { 
		gameLocal.Printf("ERROR: Wave already in progress.");
		return;
	}
	this->wave = wave; 
	this->lastWaveStart = gameLocal.GetTime();
	this->waveCount++;
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
		if (strcmp(tName, name) == 0) {
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

void TowerManager::ToggleHelpMenu(void)
{
	gameLocal.Printf("ToggleHelpMenu\n");
}

void TowerManager::SpawnWave(void)
{
	int newLevel = waveCount + 1;
	idList<idStr> monsterTypes = idList<idStr>();
	for (int i = 0; i < monsterDefinitions.Num(); i++)
	{
		auto def = monsterDefinitions[i];
		if (def->startingWave <= newLevel) {
			monsterTypes.Append(def->name);
		}
	}

	int monsterCount = 10 + (newLevel * 5);
	Wave* wave = new Wave(monsterCount, monsterTypes);
	wave->Init();
	SetWave(wave);
}

Wave::Wave(int startingMonsters, idList<idStr> monsterTypes)
{
	this->startingMonsters = startingMonsters;
	this->monstersLeft = startingMonsters;
	this->monsterTypes = monsterTypes;
	this->monsters = idList<idAI*>();
	this->started = false;
}

Wave::~Wave(void)
{
}

void Wave::Init(void)
{
	if (monsterTypes.Num() == 0) {
		gameLocal.Printf("No monsters to spawn\n");
		return;
	}

	for (int i = 0; i < monsterTypes.Num(); i++)
	{
		//gameLocal.Printf("Monster: %s\n", monsterTypes[i].c_str());
	}
	for (int i = 0; i < startingMonsters; i++)
	{
		SpawnMonster(monsterTypes[gameLocal.random.RandomInt(monsterTypes.Num())]);
	}

	started = true;
}

void Wave::Update(void)
{
	for (int i = 0; i < monsters.Num(); i++)
	{
		int minAttackDist = 250;
		int enemySpeed = 1;

		idAI* monster = monsters[i];
		if (!monster) continue;

		idEntity* enemy = monster->GetEnemy();
		if (!enemy) {
			Tower* cT = gameLocal.towerManager->FindTower(monster->GetPhysics()->GetOrigin());
			if (cT) {
				monster->SetEnemy(cT->towerEntity);
			}

			// We skip this enemy as it will be updated next frame
			continue;
		}

		idVec3 monsterPos = monster->GetPhysics()->GetOrigin();
		idVec3 targetPos = enemy->GetPhysics()->GetOrigin();

		if (monsterPos.Dist(targetPos) > minAttackDist) {
			idVec3 dir = targetPos - monsterPos;
			dir.Normalize();
			monster->GetPhysics()->SetOrigin(monsterPos + (dir * enemySpeed));
		}
	}
}

bool Wave::HasStarted(void)
{
	return started;
}

bool Wave::HasEnded(void)
{
	return monstersLeft == 0;
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
			loc = monster->GetChestPosition();
		}
		else if (monster->GetChestPosition().Dist(*tower->origin) < dist) {
			loc = monster->GetChestPosition();
		}
	}
	return loc;
}

void Wave::SpawnMonster(idStr type)
{

	int x1 = 11682;
	int x2 = 9145;
	int y1 = -7425;
	int y2 = -8998;
	int x = gameLocal.random.RandomInt(x1 - x2) + x2;
	int y = gameLocal.random.RandomInt(y1 - y2) + y2;
	idVec3 origin = idVec3(x, y, 140);
	gameLocal.Printf("Spawning monster '%s' at '%s'\n", type.c_str(), origin.ToString());

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
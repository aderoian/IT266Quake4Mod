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
	towerEntity->health = towerDef->health;
	init = true;
}

void Tower::ShootHitscan(Tower* tower, idVec3 target, const idDict* dict)
{
	if (tower->origin->Dist(target) > tower->GetRange()) return;
	idVec3 start = *tower->origin + (idVec3(0, 0, 1) * 50);
	idVec3 dir = target - start;
	dir.Normalize();
	idEntity* hit = gameLocal.HitScan(*dict, start, dir, start, tower->towerEntity, false);

	Wave* wave = gameLocal.towerManager->wave;
	if (hit && wave->IsMonsterMember(hit)) {
		tower->Damage(hit);
	}
}

void Tower::Damage(idEntity* target)
{
	if (!target) return;
	int damage = GetDamage();

	//gameLocal.Printf("Tower %s damaged monster %s for %d\n", name, target->name, damage);
	target->health -= damage;

	if (target->health <= 0) {
		//gameLocal.Printf("Monster %s has been killed by tower %s\n", target->name, name);
		target->Killed(towerEntity, towerEntity, damage, idVec3(0, 0, 0), 0);
	}
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
	if (level > towerDef->upgrades.Num() - 1) return;

	idPlayer* player = gameLocal.GetLocalPlayer();
	if (!player) return;

	TowerDef upgrade = towerDef->upgrades[level - 1];
	if (!player->inventory.ProcessTransaction(upgrade.cost, false) || !player->inventory.ProcessBuilderTransaction(1, false)) {
		gameLocal.Printf("Unable to upgrade %s to level %d, insufficient resources.", towerDef->name.c_str(), level + 1);
		return;
	}

	player->inventory.ProcessTransaction(upgrade.cost, true);
	player->inventory.ProcessBuilderTransaction(1, true);

	towerEntity->health = upgrade.health;
	level++;
}

void Tower::ShootDarkMatter(Tower* tower, idVec3 target)
{
	if (tower->origin->Dist(target) > tower->GetRange()) return;
	if (!gameLocal.GetLocalPlayer()->inventory.ProcessEnergyTransaction(10, true)) return;

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
	if (tower->origin->Dist(target) > tower->GetRange()) return;
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
	if (tower->origin->Dist(target) > tower->GetRange()) return;
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
	if (tower->origin->Dist(target) > tower->GetRange()) return;
	if (!gameLocal.GetLocalPlayer()->inventory.ProcessEnergyTransaction(2, true)) return;

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

	buildMode = 0;
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
	spawned = false;

	// Register Tower Definitions

	idList<TowerDef> darkMatterUpgrades;
	darkMatterUpgrades.Append(TowerDef("", "", ResourceCost(15000, 15000, 15000), 700, 325, 8200, 9000, Tower::ShootDarkMatter, {}));
	darkMatterUpgrades.Append(TowerDef("", "", ResourceCost(20000, 20000, 20000), 900, 350, 8400, 8000, Tower::ShootDarkMatter, {}));
	darkMatterUpgrades.Append(TowerDef("", "", ResourceCost(25000, 25000, 25000), 1100, 375, 8600, 7000, Tower::ShootDarkMatter, {}));
	darkMatterUpgrades.Append(TowerDef("", "", ResourceCost(30000, 30000, 30000), 1300, 400, 8800, 6000, Tower::ShootDarkMatter, {}));
	darkMatterUpgrades.Append(TowerDef("", "", ResourceCost(35000, 35000, 35000), 1500, 425, 9000, 5000, Tower::ShootDarkMatter, {}));
	RegisterTower(new TowerDef("dark_matter", "weapon_dmg_world", ResourceCost(10000, 10000, 10000), 500, 300, 8000, 10000, Tower::ShootDarkMatter, darkMatterUpgrades));
	
	idList<TowerDef> gauntletUpgrades;
	gauntletUpgrades.Append(TowerDef("", "", ResourceCost(750, 750, 750), 325, 55, 800, 850, Tower::ShootGauntlet, {}));
	gauntletUpgrades.Append(TowerDef("", "", ResourceCost(1500, 1500, 1500), 400, 65, 800, 800, Tower::ShootGauntlet, {}));
	gauntletUpgrades.Append(TowerDef("", "", ResourceCost(2250, 2250, 2250), 475, 75, 800, 750, Tower::ShootGauntlet, {}));
	gauntletUpgrades.Append(TowerDef("", "", ResourceCost(3000, 3000, 3000), 550, 85, 800, 700, Tower::ShootGauntlet, {}));
	gauntletUpgrades.Append(TowerDef("", "", ResourceCost(3750, 3750, 3750), 625, 95, 800, 650, Tower::ShootGauntlet, {}));
	RegisterTower(new TowerDef("gauntlet", "weapon_shotgun_world", ResourceCost(500, 500, 500), 250, 35, 750, 1000, Tower::ShootGauntlet, gauntletUpgrades));

	idList<TowerDef> grenadeLauncherUpgrades;
	grenadeLauncherUpgrades.Append(TowerDef("", "", ResourceCost(1200, 800, 500), 250, 75, 1050, 4500, Tower::ShootGrenadeLauncher, {}));
	grenadeLauncherUpgrades.Append(TowerDef("", "", ResourceCost(1600, 1000, 750), 300, 90, 1100, 4000, Tower::ShootGrenadeLauncher, {}));
	grenadeLauncherUpgrades.Append(TowerDef("", "", ResourceCost(2000, 1200, 1000), 350, 110, 1150, 3500, Tower::ShootGrenadeLauncher, {}));
	grenadeLauncherUpgrades.Append(TowerDef("", "", ResourceCost(2400, 1400, 1250), 400, 130, 1200, 3000, Tower::ShootGrenadeLauncher, {}));
	grenadeLauncherUpgrades.Append(TowerDef("", "", ResourceCost(2800, 1600, 1500), 450, 150, 1250, 2500, Tower::ShootGrenadeLauncher, {}));
	RegisterTower(new TowerDef("grenade_launcher", "weapon_grenadelauncher_world", ResourceCost(1000, 600, 400), 200, 50, 1000, 5000, Tower::ShootGrenadeLauncher, grenadeLauncherUpgrades));

	idList<TowerDef> hyperblasterUpgrades;
	hyperblasterUpgrades.Append(TowerDef("", "", ResourceCost(500, 400, 300), 150, 20, 1100, 450, Tower::ShootHyperBlaster, {}));
	hyperblasterUpgrades.Append(TowerDef("", "", ResourceCost(750, 600, 400), 200, 30, 1200, 400, Tower::ShootHyperBlaster, {}));
	hyperblasterUpgrades.Append(TowerDef("", "", ResourceCost(1000, 800, 500), 250, 40, 1300, 350, Tower::ShootHyperBlaster, {}));
	hyperblasterUpgrades.Append(TowerDef("", "", ResourceCost(1250, 1000, 600), 300, 50, 1400, 300, Tower::ShootHyperBlaster, {}));
	hyperblasterUpgrades.Append(TowerDef("", "", ResourceCost(1500, 1200, 700), 350, 60, 1500, 250, Tower::ShootHyperBlaster, {}));
	RegisterTower(new TowerDef("hyperblaster", "weapon_hyperblaster_world", ResourceCost(400, 300, 200), 100, 10, 1000, 500, Tower::ShootHyperBlaster, hyperblasterUpgrades));

	RegisterTower(new TowerDef("lightning", "weapon_lightninggun_world", ResourceCost(), 100, 10, 800, 4000, Tower::ShootLightning, {}));

	idList <TowerDef> machineGunUpgrades;
	machineGunUpgrades.Append(TowerDef("", "", ResourceCost(100, 100, 100), 200, 20, 500, 225, Tower::ShootMachineGun, {}));
	machineGunUpgrades.Append(TowerDef("", "", ResourceCost(200, 200, 200), 300, 30, 500, 200, Tower::ShootMachineGun, {}));
	machineGunUpgrades.Append(TowerDef("", "", ResourceCost(300, 300, 300), 400, 40, 500, 175, Tower::ShootMachineGun, {}));
	machineGunUpgrades.Append(TowerDef("", "", ResourceCost(400, 400, 400), 500, 50, 500, 150, Tower::ShootMachineGun, {}));
	machineGunUpgrades.Append(TowerDef("", "", ResourceCost(500, 500, 500), 600, 60, 500, 125, Tower::ShootMachineGun, {}));
	RegisterTower(new TowerDef("machine_gun", "weapon_machinegun_world", ResourceCost(10, 10, 10), 100, 10, 450, 250, Tower::ShootMachineGun, machineGunUpgrades));

	idList<TowerDef> nailgunUpgrades;
	nailgunUpgrades.Append(TowerDef("", "", ResourceCost(600, 500, 400), 200, 15, 1600, 550, Tower::ShootNailGun, {}));
	nailgunUpgrades.Append(TowerDef("", "", ResourceCost(900, 700, 600), 250, 25, 1700, 500, Tower::ShootNailGun, {}));
	nailgunUpgrades.Append(TowerDef("", "", ResourceCost(1200, 900, 800), 300, 35, 1800, 450, Tower::ShootNailGun, {}));
	nailgunUpgrades.Append(TowerDef("", "", ResourceCost(1500, 1100, 1000), 350, 45, 1900, 400, Tower::ShootNailGun, {}));
	nailgunUpgrades.Append(TowerDef("", "", ResourceCost(1800, 1300, 1200), 400, 55, 2000, 350, Tower::ShootNailGun, {}));
	RegisterTower(new TowerDef("nailgun", "weapon_nailgun_world", ResourceCost(500, 400, 300), 150, 10, 1500, 600, Tower::ShootNailGun, nailgunUpgrades));

	idList<TowerDef> napalmUpgrades;
	napalmUpgrades.Append(TowerDef("", "", ResourceCost(2500, 2000, 1500), 300, 50, 800, 4800, Tower::ShootNapalm, {}));
	napalmUpgrades.Append(TowerDef("", "", ResourceCost(3000, 2500, 2000), 350, 60, 850, 4600, Tower::ShootNapalm, {}));
	napalmUpgrades.Append(TowerDef("", "", ResourceCost(3500, 3000, 2500), 400, 70, 900, 4400, Tower::ShootNapalm, {}));
	napalmUpgrades.Append(TowerDef("", "", ResourceCost(4000, 3500, 3000), 450, 80, 950, 4200, Tower::ShootNapalm, {}));
	napalmUpgrades.Append(TowerDef("", "", ResourceCost(4500, 4000, 3500), 500, 90, 1000, 4000, Tower::ShootNapalm, {}));
	RegisterTower(new TowerDef("napalm", "weapon_napalmgun_world", ResourceCost(2000, 1500, 1000), 250, 40, 750, 5000, Tower::ShootNapalm, napalmUpgrades));

	idList<TowerDef> railgunUpgrades;
	railgunUpgrades.Append(TowerDef("", "", ResourceCost(2000, 1500, 1000), 300, 140, 2000, 4000, Tower::ShootRailgun, {}));
	railgunUpgrades.Append(TowerDef("", "", ResourceCost(2500, 2000, 1500), 400, 150, 2100, 3500, Tower::ShootRailgun, {}));
	railgunUpgrades.Append(TowerDef("", "", ResourceCost(3000, 2500, 2000), 500, 160, 2200, 3000, Tower::ShootRailgun, {}));
	railgunUpgrades.Append(TowerDef("", "", ResourceCost(3500, 3000, 2500), 600, 170, 2300, 2500, Tower::ShootRailgun, {}));
	railgunUpgrades.Append(TowerDef("", "", ResourceCost(4000, 3500, 3000), 700, 180, 2400, 2000, Tower::ShootRailgun, {}));
	RegisterTower(new TowerDef("railgun", "weapon_railgun_world", ResourceCost(1500, 1000, 500), 200, 130, 2900, 4500, Tower::ShootRailgun, railgunUpgrades));
	
	idList<TowerDef> rocketLauncherUpgrades;
	rocketLauncherUpgrades.Append(TowerDef("", "", ResourceCost(2000, 1500, 1000), 400, 50, 1200, 4000, Tower::ShootRocketLauncher, {}));
	rocketLauncherUpgrades.Append(TowerDef("", "", ResourceCost(2500, 2000, 1500), 500, 60, 1300, 3500, Tower::ShootRocketLauncher, {}));
	rocketLauncherUpgrades.Append(TowerDef("", "", ResourceCost(3000, 2500, 2000), 600, 70, 1400, 3000, Tower::ShootRocketLauncher, {}));
	rocketLauncherUpgrades.Append(TowerDef("", "", ResourceCost(3500, 3000, 2500), 700, 80, 1500, 2500, Tower::ShootRocketLauncher, {}));
	rocketLauncherUpgrades.Append(TowerDef("", "", ResourceCost(4000, 3500, 3000), 800, 90, 1600, 2000, Tower::ShootRocketLauncher, {}));
	RegisterTower(new TowerDef("rocketlauncher", "weapon_rocketlauncher_world", ResourceCost(1500, 1000, 500), 200, 40, 1000, 4500, Tower::ShootRocketLauncher, rocketLauncherUpgrades));

	// Economy Towers
	idList<TowerDef> goldGeneratorUpgrades;
	goldGeneratorUpgrades.Append(TowerDef("", "", ResourceCost(1000, 1000, 1000), 100, 5, 0, 900, Tower::GenerateGold, {}));
	goldGeneratorUpgrades.Append(TowerDef("", "", ResourceCost(2000, 2000, 2000), 200, 6, 0, 800, Tower::GenerateGold, {}));
	goldGeneratorUpgrades.Append(TowerDef("", "", ResourceCost(3000, 3000, 3000), 300, 7, 0, 700, Tower::GenerateGold, {}));
	goldGeneratorUpgrades.Append(TowerDef("", "", ResourceCost(4000, 4000, 4000), 400, 8, 0, 600, Tower::GenerateGold, {}));
	goldGeneratorUpgrades.Append(TowerDef("", "", ResourceCost(5000, 5000, 5000), 500, 9, 0, 250, Tower::GenerateGold, {}));
	RegisterTower(new TowerDef("gold_generator", "models/pick_ups/sp_pickups/sp_darkmatter.lwo", ResourceCost(100, 100, 100), 150, 1, 0, 1000, Tower::GenerateGold, goldGeneratorUpgrades));

	idList<TowerDef> energyGeneratorUpgrades;
	energyGeneratorUpgrades.Append(TowerDef("", "", ResourceCost(1000, 1000, 1000), 100, 5, 0, 900, Tower::GenerateEnergy, {}));
	energyGeneratorUpgrades.Append(TowerDef("", "", ResourceCost(2000, 2000, 2000), 200, 6, 0, 800, Tower::GenerateEnergy, {}));
	energyGeneratorUpgrades.Append(TowerDef("", "", ResourceCost(3000, 3000, 3000), 300, 7, 0, 700, Tower::GenerateEnergy, {}));
	energyGeneratorUpgrades.Append(TowerDef("", "", ResourceCost(4000, 4000, 4000), 400, 8, 0, 600, Tower::GenerateEnergy, {}));
	energyGeneratorUpgrades.Append(TowerDef("", "", ResourceCost(5000, 5000, 5000), 500, 9, 0, 250, Tower::GenerateEnergy, {}));
	RegisterTower(new TowerDef("energy_generator", "models/pick_ups/sp_pickups/sp_darkmatter.lwo", ResourceCost(100, 100, 100), 150, 1, 0, 1000, Tower::GenerateEnergy, energyGeneratorUpgrades));

	idList<TowerDef> stoneGeneratorUpgrades;
	stoneGeneratorUpgrades.Append(TowerDef("", "", ResourceCost(1000, 1000, 1000), 100, 5, 0, 900, Tower::GenerateStone, {}));
	stoneGeneratorUpgrades.Append(TowerDef("", "", ResourceCost(2000, 2000, 2000), 200, 6, 0, 800, Tower::GenerateStone, {}));
	stoneGeneratorUpgrades.Append(TowerDef("", "", ResourceCost(3000, 3000, 3000), 300, 7, 0, 700, Tower::GenerateStone, {}));
	stoneGeneratorUpgrades.Append(TowerDef("", "", ResourceCost(4000, 4000, 4000), 400, 8, 0, 600, Tower::GenerateStone, {}));
	stoneGeneratorUpgrades.Append(TowerDef("", "", ResourceCost(5000, 5000, 5000), 500, 9, 0, 250, Tower::GenerateStone, {}));
	RegisterTower(new TowerDef("stone_generator", "models/pick_ups/sp_pickups/sp_darkmatter.lwo", ResourceCost(100, 100, 100), 150, 1, 0, 1000, Tower::GenerateStone, stoneGeneratorUpgrades));

	idList<TowerDef> woodGeneratorUpgrades;
	woodGeneratorUpgrades.Append(TowerDef("", "", ResourceCost(1000, 1000, 1000), 100, 5, 0, 900, Tower::GenerateWood, {}));
	woodGeneratorUpgrades.Append(TowerDef("", "", ResourceCost(2000, 2000, 2000), 200, 6, 0, 800, Tower::GenerateWood, {}));
	woodGeneratorUpgrades.Append(TowerDef("", "", ResourceCost(3000, 3000, 3000), 300, 7, 0, 700, Tower::GenerateWood, {}));
	woodGeneratorUpgrades.Append(TowerDef("", "", ResourceCost(4000, 4000, 4000), 400, 8, 0, 600, Tower::GenerateWood, {}));
	woodGeneratorUpgrades.Append(TowerDef("", "", ResourceCost(5000, 5000, 5000), 500, 9, 0, 250, Tower::GenerateWood, {}));
	RegisterTower(new TowerDef("wood_generator", "models/pick_ups/sp_pickups/sp_darkmatter.lwo", ResourceCost(100, 100, 100), 150, 1, 0, 1000, Tower::GenerateWood, woodGeneratorUpgrades));

	idList<TowerDef> builderGeneratorUpgrades;
	builderGeneratorUpgrades.Append(TowerDef("", "", ResourceCost(1000, 1000, 1000), 100, 1, 0, 900, Tower::GenerateBuilder, {}));
	builderGeneratorUpgrades.Append(TowerDef("", "", ResourceCost(2000, 2000, 2000), 200, 1, 0, 800, Tower::GenerateBuilder, {}));
	builderGeneratorUpgrades.Append(TowerDef("", "", ResourceCost(3000, 3000, 3000), 300, 2, 0, 700, Tower::GenerateBuilder, {}));
	builderGeneratorUpgrades.Append(TowerDef("", "", ResourceCost(4000, 4000, 4000), 400, 2, 0, 600, Tower::GenerateBuilder, {}));
	builderGeneratorUpgrades.Append(TowerDef("", "", ResourceCost(5000, 5000, 5000), 500, 3, 0, 250, Tower::GenerateBuilder, {}));
	RegisterTower(new TowerDef("builder_generator", "models/pick_ups/sp_pickups/sp_darkmatter.lwo", ResourceCost(100, 100, 100), 150, 1, 0, 1000, Tower::GenerateBuilder, builderGeneratorUpgrades));

	// Register Monster Definitions
	RegisterMonster(new WaveMonsterDef("monster_berserker", 300, 10, 1));
	RegisterMonster(new WaveMonsterDef("monster_gladiator", 400, 10, 2));
	RegisterMonster(new WaveMonsterDef("monster_grunt", 500, 10, 2));
	RegisterMonster(new WaveMonsterDef("monster_gunner", 700, 10, 3));
	RegisterMonster(new WaveMonsterDef("monster_hh_tank", 900, 10, 3));
	RegisterMonster(new WaveMonsterDef("monster_iron_maiden", 1100, 10, 4));
	RegisterMonster(new WaveMonsterDef("monster_lt_tank", 1500, 10, 4));
	RegisterMonster(new WaveMonsterDef("monster_network_guardian", 5000, 10, 5));
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
	if (!spawned) {
		if (gameLocal.GetLocalPlayer()) {
			gameLocal.GetLocalPlayer()->GetPhysics()->SetOrigin(idVec3(10031, -8275, 128));
			spawned = true;
			gameLocal.GetLocalPlayer()->godmode = true;
		}
		else
		{
			return;
		}
	}

	if (!gameStarted) {
		if (towers.Num() > 3) {
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
	buildMode = (buildMode + 1) % 3;
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

void TowerManager::UpgradeTower(idVec3 origin)
{
	Tower* tower = FindTower(origin);
	if (!tower) return;
	tower->Upgrade();
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

bool Wave::IsMonsterMember(idEntity* monster)
{
	if (!monster) return false;

	auto mon = dynamic_cast<idAI*>(monster);
	if (!mon) return false;

	return monsters.FindIndex(mon) != -1;
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

	idList<idStr> name;
	monster->name.Split(name, '-');
	if (name.Num() < 2) return;
	int damage = gameLocal.towerManager->monsterDefinitions[name[0]]->baseDamage * (gameLocal.towerManager->waveCount / 2);
	auto tower = gameLocal.towerManager->FindTower(target->name);

	if (tower) {
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

	WaveMonsterDef* monsterDef = gameLocal.towerManager->monsterDefinitions[type];
	if (!monsterDef) return;

	idDict dict;
	dict.Set("classname", type);
	dict.Set("origin", origin.ToString());
	dict.Set("name", va("%s-%d", monsterDef->name.c_str(), gameLocal.towerManager->entityId++));
	dict.Set("target", gameLocal.towerManager->towers[0]->name);

	idEntity* newEnt;
	gameLocal.SpawnEntityDef(dict, &newEnt);

	if (newEnt) {
		gameLocal.Printf("spawned entity '%s'\n", newEnt->name.c_str());

		idAI* ai = dynamic_cast<idAI*>(newEnt);
		if (!ai) return;
		ai->health = monsterDef->baseHealth * (gameLocal.towerManager->waveCount / 2.0);
		monsters.Append(ai);
	}
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
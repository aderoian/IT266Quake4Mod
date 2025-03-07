#ifndef __TOWER_H__
#define __TOWER_H__

class Tower;

class Wave {
public:
	int startingMonsters;
	int monstersLeft;
	idList<idAI*> monsters;
public:
	Wave(int startingMonsters, idList<idStr> monsterTypes);
	~Wave(void);
	void Init(void);
	void Update(void);
	bool HasStarted(void);
	bool HasEnded(void);

	bool IsMonsterMember(idAI* monster);
	void OnMonsterKilled(idAI* monster);
	void OnAttack(idAI* monster, idEntity* target, idEntity* projectile);

	idVec3 GetNearestMonster(Tower* tower);

private:
	idList<idStr> monsterTypes;
	bool started;

private:
	void SpawnMonster(idStr type);
};

struct ResourceCost {
	int wood;
	int stone;
	int gold;

	ResourceCost() {
		this->wood = 0;
		this->stone = 0;
		this->gold = 0;
	}

	ResourceCost(int wood, int stone, int gold) {
		this->wood = wood;
		this->stone = stone;
		this->gold = gold;
	}
};

struct TowerUpgrade {
	idStr name;
	ResourceCost cost;
	int damageInc;
	int rangeInc;
	int shootDelayDec;

	TowerUpgrade() {
		this->name = "";
		this->cost = ResourceCost(0, 0, 0);
		this->damageInc = 0;
		this->rangeInc = 0;
		this->shootDelayDec = 0;
	}
};

typedef void (*TowerShootFunc_t)(Tower* tower, idVec3 target);

struct TowerDef {

	idStr name;
	idStr model;
	ResourceCost cost;
	int damage;
	int range;
	int shootDelay;
	TowerShootFunc_t shootFunc;
	idList<TowerDef> upgrades;

	TowerDef() {
		this->name = "";
		this->model = "";
		this->cost = ResourceCost(0, 0, 0);
		this->damage = 0;
		this->range = 0;
		this->shootDelay = 0;
		this->shootFunc = nullptr;
		this->upgrades = idList<TowerDef>();
	}

	TowerDef(idStr name, idStr model, ResourceCost cost, int damage, int range, int shootDelay, TowerShootFunc_t shootFunc, idList<TowerDef> upgrades) {
		this->name = name;
		this->model = model;
		this->cost = cost;
		this->damage = damage;
		this->range = range;
		this->shootDelay = shootDelay;
		this->shootFunc = shootFunc;
		this->upgrades = upgrades;
	}

	int GetDamage(int level);
	int GetRange(int level);
	int GetShootDelay(int level);
};

struct WaveMonsterDef {
	idStr name;
	int baseHealth;
	int baseDamage;
	int startingWave;

	WaveMonsterDef() {
		this->name = "";
		this->baseHealth = 0;
		this->baseDamage = 0;
		this->startingWave = 0;
	}

	WaveMonsterDef(idStr name, int baseHealth, int baseDamage, int startingWave) {
		this->name = name;
		this->baseHealth = baseHealth;
		this->baseDamage = baseDamage;
		this->startingWave = startingWave;
	}
};

// Write only dictionary for tower defs because idDict doesn't support generic types
template <class T>
class DefList {
public:
	DefList(void);
	~DefList(void);

	const T operator[](const char * name) {
		return GetDef(name);
	}
	const T operator[](int index) {
		return defs[index];
	}

	void AddDef(T def);
	int Num();

private:
	idList <T> defs;
	idDict keyMap;

	T GetDef(const char * name);
};

class Tower {

public:
	int id;
	idPlayer* owner;
	TowerDef* towerDef;
	idVec3* origin;
	char* name;
	idEntity* towerEntity;
	int level;

public:
	Tower(idPlayer* owner, TowerDef* tower, idVec3* origin);
	~Tower(void);
	//void Init(idVec3 origin, int id);
	void Update(void);
	int GetDamage(void);
	int GetRange(void);

	bool CanShoot(void);
	void Shoot(void);
	void ForceShoot(); // Shoots without checking if we can actually shoot

	void Upgrade(void);

	static void ShootDarkMatter(Tower* tower, idVec3 target);
	static void ShootGauntlet(Tower* tower, idVec3 target);
	static void ShootGrenadeLauncher(Tower* tower, idVec3 target);
	static void ShootHyperBlaster(Tower* tower, idVec3 target);
	static void ShootLightning(Tower* tower, idVec3 target);
	static void ShootMachineGun(Tower* tower, idVec3 target);
	static void ShootNailGun(Tower* tower, idVec3 target);
	static void ShootNapalm(Tower* tower, idVec3 target);
	static void ShootRailgun(Tower* tower, idVec3 target);
	static void ShootRocketLauncher(Tower* tower, idVec3 target);
	static void GenerateGold(Tower* tower, idVec3 target);
	static void GenerateEnergy(Tower* tower, idVec3 target);
	static void GenerateStone(Tower* tower, idVec3 target);
	static void GenerateWood(Tower* tower, idVec3 target);
	static void GenerateBuilder(Tower* tower, idVec3 target);

private:
	bool init;
	int lastShot;

private:
	void SpawnTower();

	static void ShootHitscan(Tower* tower, idVec3 target, const idDict* dict);
};

class TowerManager {
public:
	int entityId;

	bool buildMode;
	TowerDef* buildTower;
	DefList<TowerDef*> towerDefinitions;
	DefList<WaveMonsterDef*> monsterDefinitions;

	idList<Tower*> towers;
	bool gameStarted; 

	Wave* wave;
	int waveCount;

public:
	TowerManager(void);
	~TowerManager(void);

	void Init(void);
	void RegisterTower(TowerDef* def);
	void RegisterMonster(WaveMonsterDef* def);

	void Update(void);
	void AddTower(Tower* tower);
	bool CanTowersShoot(void);

	void ToggleBuild(void);
	void SetBuildTowerFromKey(int key);
	void BuildTower(idVec3 origin);
	void DestroyTower(Tower* tower);

	void SetWave(Wave* wave);

	Tower* FindTower(int id);
	Tower* FindTower(idVec3 origin);
	Tower* FindTower(const char* name);

	static void ArgCompletion_TowerDefs(const idCmdArgs& args, void(*callback)(const char* s));
	static void ToggleHelpMenu(void);

private:
	int lastWaveStart;
	int lastWaveEnd;
	int waveDelay;

private:
	void SpawnWave(void);
};

#endif // __TOWER_H__
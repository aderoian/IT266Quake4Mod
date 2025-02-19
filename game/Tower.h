#ifndef __TOWER_H__
#define __TOWER_H__

class Wave {
public:
	Wave(void);
	~Wave(void);
	void Init(void);
	void Update(void);
	bool HasStarted(void);
	bool HasEnded(void);

public:
	int startingMonsters;
	int monstersLeft;

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

struct TowerDef {
	idStr name;
	idStr model;
	ResourceCost cost;
	int damage;
	int range;
	int shootDelay;

	TowerDef() {
		this->name = "";
		this->model = "";
		this->cost = ResourceCost(0, 0, 0);
		this->damage = 0;
		this->range = 0;
		this->shootDelay = 0;
	}

	TowerDef(idStr name, idStr model, ResourceCost cost, int damage, int range, int shootDelay) {
		this->name = name;
		this->model = model;
		this->cost = cost;
		this->damage = damage;
		this->range = range;
		this->shootDelay = shootDelay;
	}
};

// Write only dictionary for tower defs because idDict doesn't support generic types
class TowerDefList {
public:
	TowerDefList(void);
	~TowerDefList(void);

	const TowerDef* operator[](const char * name) {
		return GetDef(name);
	}
	const TowerDef* operator[](int index) {
		return towerDefs[index];
	}

	void AddDef(TowerDef* def);
	int Num();

private:
	idList <TowerDef*> towerDefs;
	idDict keyMap;

	TowerDef* GetDef(const char * name);
};

class Tower {

public:
	int id;

public:
	Tower(idPlayer* owner, idStr tower);
	~Tower(void);
	void Init(idVec3 origin);
	void Update(void);
	idVec3 GetOrigin(void);

	bool CanShoot(void);
	void Shoot(void);
	void ForceShoot(void); // Shoots without checking if we can actually shoot

private:
	idPlayer* owner;
	idStr tower;
	idVec3 origin;
	idEntity* towerEntity;

	bool init;
	
	int shootDelay;

private:
	void SpawnTower();
};

class TowerManager {
public:
	int towerId;

	bool buildMode;
	const char* buildTower;
	TowerDefList towerDefinitions;

public:
	TowerManager(void);
	~TowerManager(void);

	void Init(void);
	void Register(TowerDef* def);
	void Update(void);
	int AddTower(Tower* tower);
	bool CanTowersShoot(void);

	void ToggleBuild(void);
	void BuildTower(idVec3 origin);

	static void ArgCompletion_TowerDefs(const idCmdArgs& args, void(*callback)(const char* s));

private:
	idList<Tower*> towers;
	Wave* wave;

	int lastWaveStart;
	int lastWaveEnd;
	int waveDelay;
};

#endif // __TOWER_H__
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

class TowerManager {
public:
	bool buildMode;
	char* buildTower;

public:
	TowerManager(void);
	~TowerManager(void);

	void Init(void);
	void Update(void);
	void AddTower(Tower* &tower);
	bool CanTowersShoot(void);

private:
	idList<Tower*> towers;
	Wave* wave;

	int lastWaveStart;
	int lastWaveEnd;
	int waveDelay;
};

class Tower {

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

#endif // __TOWER_H__
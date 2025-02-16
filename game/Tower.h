#ifndef __TOWER_H__
#define __TOWER_H__

class TowerManager {
public:
	TowerManager(void);
	~TowerManager(void);

	void Init(void);

	void Update(void);

};

class Tower {

public:
	Tower(idPlayer* owner, idStr tower);
	~Tower(void);
	void Init(idVec3 origin);
	void SpawnTower();
	void Update(void);
	idVec3 GetOrigin(void);

private:
	idPlayer* owner;
	idStr tower;
	idVec3 origin;
};

#endif // __TOWER_H__
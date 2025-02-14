#include "../idlib/precompiled.h"
#pragma hdrstop

#include "Tower.h"
#include "Game_local.h"

TowerManager::TowerManager()
{
	gameLocal.Printf("Tower manager created...\n");
}

TowerManager::~TowerManager()
{
}

void TowerManager::Init()
{
	gameLocal.Printf("Tower manager initialized...\n");
}

void TowerManager::Update()
{
}

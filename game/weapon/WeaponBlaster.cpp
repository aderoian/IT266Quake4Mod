#include "../../idlib/precompiled.h"
#pragma hdrstop

#include "../Game_local.h"
#include "../Weapon.h"

#define BLASTER_SPARM_CHARGEGLOW		6

class rvWeaponBlaster : public rvWeapon {
public:

	CLASS_PROTOTYPE( rvWeaponBlaster );

	rvWeaponBlaster ( void );

	virtual void		Spawn				( void );
	void				Save				( idSaveGame *savefile ) const;
	void				Restore				( idRestoreGame *savefile );
	void				PreSave		( void );
	void				PostSave	( void );
	virtual void Attack(bool altAttack, int num_attacks, float spread, float fuseOffset, float power);

protected:

	bool				UpdateAttack		( void );
	bool				UpdateFlashlight	( void );
	void				Flashlight			( bool on );
	idVec3				Tower_Raycast(const idDict& dict, const idVec3& muzzleOrigin, const idMat3& muzzleAxis, int num_hitscans, float spread, float power);

private:

	int					chargeTime;
	int					chargeDelay;
	idVec2				chargeGlow;
	bool				fireForced;
	int					fireHeldTime;

	stateResult_t		State_Raise				( const stateParms_t& parms );
	stateResult_t		State_Lower				( const stateParms_t& parms );
	stateResult_t		State_Idle				( const stateParms_t& parms );
	stateResult_t		State_Charge			( const stateParms_t& parms );
	stateResult_t		State_Charged			( const stateParms_t& parms );
	stateResult_t		State_Fire				( const stateParms_t& parms );
	stateResult_t		State_Flashlight		( const stateParms_t& parms );
	
	CLASS_STATES_PROTOTYPE ( rvWeaponBlaster );
};

CLASS_DECLARATION( rvWeapon, rvWeaponBlaster )
END_CLASS

/*
================
rvWeaponBlaster::rvWeaponBlaster
================
*/
rvWeaponBlaster::rvWeaponBlaster ( void ) {
}

/*
================
rvWeaponBlaster::UpdateFlashlight
================
*/
bool rvWeaponBlaster::UpdateFlashlight ( void ) {
	if ( !wsfl.flashlight ) {
		return false;
	}
	
	SetState ( "Flashlight", 0 );
	return true;		
}

/*
================
rvWeaponBlaster::Flashlight
================
*/
void rvWeaponBlaster::Flashlight ( bool on ) {
	owner->Flashlight ( on );
	
	if ( on ) {
		worldModel->ShowSurface ( "models/weapons/blaster/flare" );
		viewModel->ShowSurface ( "models/weapons/blaster/flare" );
	} else {
		worldModel->HideSurface ( "models/weapons/blaster/flare" );
		viewModel->HideSurface ( "models/weapons/blaster/flare" );
	}
}

idVec3 rvWeaponBlaster::Tower_Raycast(const idDict& dict, const idVec3& muzzleOrigin, const idMat3& muzzleAxis, int num_hitscans, float spread, float power)
{
	if (!gameLocal.towerManager->buildMode) return idVec3(0, 0, 0);

	idVec3  fxOrigin;
	idMat3  fxAxis;
	int		i;
	float	ang;
	float	spin;
	idVec3	dir;
	int		areas[2];

	idBitMsg	msg;
	byte		msgBuf[MAX_GAME_MESSAGE_SIZE];

	GetGlobalJointTransform(true, flashJointView, fxOrigin, fxAxis, dict.GetVector("fxOriginOffset"));

	float spreadRad = DEG2RAD(spread);
	idVec3 end;
	ang = idMath::Sin(spreadRad * gameLocal.random.RandomFloat());
	spin = (float)DEG2RAD(360.0f) * gameLocal.random.RandomFloat();
	//RAVEN BEGIN
	//asalmon: xbox must use the muzzleAxis so the aim can be adjusted for aim assistance
#ifdef _XBOX
	dir = muzzleAxis[0] + muzzleAxis[2] * (ang * idMath::Sin(spin)) - muzzleAxis[1] * (ang * idMath::Cos(spin));
#else
	dir = playerViewAxis[0] + playerViewAxis[2] * (ang * idMath::Sin(spin)) - playerViewAxis[1] * (ang * idMath::Cos(spin));
#endif
	//RAVEN END
	dir.Normalize();

	return gameLocal.Raycast(dict, muzzleOrigin, dir, fxOrigin, owner, false, 1.0f, NULL, areas);
}

/*
================
rvWeaponBlaster::UpdateAttack
================
*/
bool rvWeaponBlaster::UpdateAttack ( void ) {
	// Clear fire forced
	if ( fireForced ) {
		if ( !wsfl.attack ) {
			fireForced = false;
		} else {
			return false;
		}
	}

	// If the player is pressing the fire button and they have enough ammo for a shot
	// then start the shooting process.
	if ( wsfl.attack && gameLocal.time >= nextAttackTime ) {
		// Save the time which the fire button was pressed
		if ( fireHeldTime == 0 ) {		
			nextAttackTime = gameLocal.time + (fireRate * owner->PowerUpModifier ( PMOD_FIRERATE ));
			fireHeldTime   = gameLocal.time;
			viewModel->SetShaderParm ( BLASTER_SPARM_CHARGEGLOW, chargeGlow[0] );
		}
	}		

	// If they have the charge mod and they have overcome the initial charge 
	// delay then transition to the charge state.
	if ( fireHeldTime != 0 ) {
		if ( gameLocal.time - fireHeldTime > chargeDelay ) {
			SetState ( "Charge", 4 );
			return true;
		}

		// If the fire button was let go but was pressed at one point then 
		// release the shot.
		if ( !wsfl.attack ) {
			idPlayer * player = gameLocal.GetLocalPlayer();
			if( player )	{
			
				if( player->GuiActive())	{
					//make sure the player isn't looking at a gui first
					SetState ( "Lower", 0 );
				} else {
					SetState ( "Fire", 0 );
				}
			}
			return true;
		}
	}
	
	return false;
}

/*
================
rvWeaponBlaster::Spawn
================
*/
void rvWeaponBlaster::Spawn ( void ) {
	viewModel->SetShaderParm ( BLASTER_SPARM_CHARGEGLOW, 0 );
	SetState ( "Raise", 0 );
	
	chargeGlow   = spawnArgs.GetVec2 ( "chargeGlow" );
	chargeTime   = SEC2MS ( spawnArgs.GetFloat ( "chargeTime" ) );
	chargeDelay  = SEC2MS ( spawnArgs.GetFloat ( "chargeDelay" ) );

	fireHeldTime		= 0;
	fireForced			= false;
			
	Flashlight ( owner->IsFlashlightOn() );
}

/*
================
rvWeaponBlaster::Save
================
*/
void rvWeaponBlaster::Save ( idSaveGame *savefile ) const {
	savefile->WriteInt ( chargeTime );
	savefile->WriteInt ( chargeDelay );
	savefile->WriteVec2 ( chargeGlow );
	savefile->WriteBool ( fireForced );
	savefile->WriteInt ( fireHeldTime );
}

/*
================
rvWeaponBlaster::Restore
================
*/
void rvWeaponBlaster::Restore ( idRestoreGame *savefile ) {
	savefile->ReadInt ( chargeTime );
	savefile->ReadInt ( chargeDelay );
	savefile->ReadVec2 ( chargeGlow );
	savefile->ReadBool ( fireForced );
	savefile->ReadInt ( fireHeldTime );
}

/*
================
rvWeaponBlaster::PreSave
================
*/
void rvWeaponBlaster::PreSave ( void ) {

	SetState ( "Idle", 4 );

	StopSound( SND_CHANNEL_WEAPON, 0);
	StopSound( SND_CHANNEL_BODY, 0);
	StopSound( SND_CHANNEL_ITEM, 0);
	StopSound( SND_CHANNEL_ANY, false );
	
}

/*
================
rvWeaponBlaster::PostSave
================
*/
void rvWeaponBlaster::PostSave ( void ) {
}

/*
===============================================================================

	States 

===============================================================================
*/

CLASS_STATES_DECLARATION ( rvWeaponBlaster )
	STATE ( "Raise",						rvWeaponBlaster::State_Raise )
	STATE ( "Lower",						rvWeaponBlaster::State_Lower )
	STATE ( "Idle",							rvWeaponBlaster::State_Idle)
	STATE ( "Charge",						rvWeaponBlaster::State_Charge )
	STATE ( "Charged",						rvWeaponBlaster::State_Charged )
	STATE ( "Fire",							rvWeaponBlaster::State_Fire )
	STATE ( "Flashlight",					rvWeaponBlaster::State_Flashlight )
END_CLASS_STATES

/*
================
rvWeaponBlaster::State_Raise
================
*/
stateResult_t rvWeaponBlaster::State_Raise( const stateParms_t& parms ) {
	enum {
		RAISE_INIT,
		RAISE_WAIT,
	};	
	switch ( parms.stage ) {
		case RAISE_INIT:			
			SetStatus ( WP_RISING );
			PlayAnim( ANIMCHANNEL_ALL, "raise", parms.blendFrames );
			return SRESULT_STAGE(RAISE_WAIT);
			
		case RAISE_WAIT:
			if ( AnimDone ( ANIMCHANNEL_ALL, 4 ) ) {
				SetState ( "Idle", 4 );
				return SRESULT_DONE;
			}
			if ( wsfl.lowerWeapon ) {
				SetState ( "Lower", 4 );
				return SRESULT_DONE;
			}
			return SRESULT_WAIT;
	}
	return SRESULT_ERROR;	
}

/*
================
rvWeaponBlaster::State_Lower
================
*/
stateResult_t rvWeaponBlaster::State_Lower ( const stateParms_t& parms ) {
	enum {
		LOWER_INIT,
		LOWER_WAIT,
		LOWER_WAITRAISE
	};	
	switch ( parms.stage ) {
		case LOWER_INIT:
			SetStatus ( WP_LOWERING );
			PlayAnim( ANIMCHANNEL_ALL, "putaway", parms.blendFrames );
			return SRESULT_STAGE(LOWER_WAIT);
			
		case LOWER_WAIT:
			if ( AnimDone ( ANIMCHANNEL_ALL, 0 ) ) {
				SetStatus ( WP_HOLSTERED );
				return SRESULT_STAGE(LOWER_WAITRAISE);
			}
			return SRESULT_WAIT;
	
		case LOWER_WAITRAISE:
			if ( wsfl.raiseWeapon ) {
				SetState ( "Raise", 0 );
				return SRESULT_DONE;
			}
			return SRESULT_WAIT;
	}
	return SRESULT_ERROR;
}

/*
================
rvWeaponBlaster::State_Idle
================
*/
stateResult_t rvWeaponBlaster::State_Idle ( const stateParms_t& parms ) {	
	enum {
		IDLE_INIT,
		IDLE_WAIT,
	};	
	switch ( parms.stage ) {
		case IDLE_INIT:			
			SetStatus ( WP_READY );
			PlayCycle( ANIMCHANNEL_ALL, "idle", parms.blendFrames );
			return SRESULT_STAGE ( IDLE_WAIT );
			
		case IDLE_WAIT:
			if ( wsfl.lowerWeapon ) {
				SetState ( "Lower", 4 );
				return SRESULT_DONE;
			}
			
			if ( UpdateFlashlight ( ) ) { 
				return SRESULT_DONE;
			}
			if ( UpdateAttack ( ) ) {
				return SRESULT_DONE;
			}
			return SRESULT_WAIT;
	}
	return SRESULT_ERROR;
}

/*
================
rvWeaponBlaster::State_Charge
================
*/
stateResult_t rvWeaponBlaster::State_Charge ( const stateParms_t& parms ) {
	enum {
		CHARGE_INIT,
		CHARGE_WAIT,
	};	
	switch ( parms.stage ) {
		case CHARGE_INIT:
			viewModel->SetShaderParm ( BLASTER_SPARM_CHARGEGLOW, chargeGlow[0] );
			StartSound ( "snd_charge", SND_CHANNEL_ITEM, 0, false, NULL );
			PlayCycle( ANIMCHANNEL_ALL, "charging", parms.blendFrames );
			return SRESULT_STAGE ( CHARGE_WAIT );
			
		case CHARGE_WAIT:	
			if ( gameLocal.time - fireHeldTime < chargeTime ) {
				float f;
				f = (float)(gameLocal.time - fireHeldTime) / (float)chargeTime;
				f = chargeGlow[0] + f * (chargeGlow[1] - chargeGlow[0]);
				f = idMath::ClampFloat ( chargeGlow[0], chargeGlow[1], f );
				viewModel->SetShaderParm ( BLASTER_SPARM_CHARGEGLOW, f );
				
				if ( !wsfl.attack ) {
					SetState ( "Fire", 0 );
					return SRESULT_DONE;
				}
				
				return SRESULT_WAIT;
			} 
			SetState ( "Charged", 4 );
			return SRESULT_DONE;
	}
	return SRESULT_ERROR;	
}

/*
================
rvWeaponBlaster::State_Charged
================
*/
stateResult_t rvWeaponBlaster::State_Charged ( const stateParms_t& parms ) {
	enum {
		CHARGED_INIT,
		CHARGED_WAIT,
	};	
	switch ( parms.stage ) {
		case CHARGED_INIT:		
			viewModel->SetShaderParm ( BLASTER_SPARM_CHARGEGLOW, 1.0f  );

			StopSound ( SND_CHANNEL_ITEM, false );
			StartSound ( "snd_charge_loop", SND_CHANNEL_ITEM, 0, false, NULL );
			StartSound ( "snd_charge_click", SND_CHANNEL_BODY, 0, false, NULL );
			return SRESULT_STAGE(CHARGED_WAIT);
			
		case CHARGED_WAIT:
			if ( !wsfl.attack ) {
				fireForced = true;
				SetState ( "Fire", 0 );
				return SRESULT_DONE;
			}
			return SRESULT_WAIT;
	}
	return SRESULT_ERROR;
}

/*
================
rvWeaponBlaster::State_Fire
================
*/
stateResult_t rvWeaponBlaster::State_Fire ( const stateParms_t& parms ) {
	enum {
		FIRE_INIT,
		FIRE_WAIT,
	};	
	switch ( parms.stage ) {
		case FIRE_INIT:	

			StopSound ( SND_CHANNEL_ITEM, false );
			viewModel->SetShaderParm ( BLASTER_SPARM_CHARGEGLOW, 0 );
			//don't fire if we're targeting a gui.
			idPlayer* player;
			player = gameLocal.GetLocalPlayer();

			//make sure the player isn't looking at a gui first
			if( player && player->GuiActive() )	{
				fireHeldTime = 0;
				SetState ( "Lower", 0 );
				return SRESULT_DONE;
			}

			if( player && !player->CanFire() )	{
				fireHeldTime = 0;
				SetState ( "Idle", 4 );
				return SRESULT_DONE;
			}


	
			if ( gameLocal.time - fireHeldTime > chargeTime ) {	
				Attack ( true, 1, spread, 0, 1.0f );
				PlayEffect ( "fx_chargedflash", barrelJointView, false );
				PlayAnim( ANIMCHANNEL_ALL, "chargedfire", parms.blendFrames );
			} else {
				Attack ( false, 1, spread, 0, 1.0f );
				PlayEffect ( "fx_normalflash", barrelJointView, false );
				PlayAnim( ANIMCHANNEL_ALL, "fire", parms.blendFrames );
			}
			fireHeldTime = 0;
			
			return SRESULT_STAGE(FIRE_WAIT);
		
		case FIRE_WAIT:
			if ( AnimDone ( ANIMCHANNEL_ALL, 4 ) ) {
				SetState ( "Idle", 4 );
				return SRESULT_DONE;
			}
			if ( UpdateFlashlight ( ) || UpdateAttack ( ) ) {
				return SRESULT_DONE;
			}
			return SRESULT_WAIT;
	}			
	return SRESULT_ERROR;
}

/*
================
rvWeaponBlaster::State_Flashlight
================
*/
stateResult_t rvWeaponBlaster::State_Flashlight ( const stateParms_t& parms ) {
	enum {
		FLASHLIGHT_INIT,
		FLASHLIGHT_WAIT,
	};	
	switch ( parms.stage ) {
		case FLASHLIGHT_INIT:			
			SetStatus ( WP_FLASHLIGHT );
			// Wait for the flashlight anim to play		
			PlayAnim( ANIMCHANNEL_ALL, "flashlight", 0 );
			return SRESULT_STAGE ( FLASHLIGHT_WAIT );
			
		case FLASHLIGHT_WAIT:
			if ( !AnimDone ( ANIMCHANNEL_ALL, 4 ) ) {
				return SRESULT_WAIT;
			}
			
			if ( owner->IsFlashlightOn() ) {
				Flashlight ( false );
			} else {
				Flashlight ( true );
			}
			
			SetState ( "Idle", 4 );
			return SRESULT_DONE;
	}
	return SRESULT_ERROR;
}

void rvWeaponBlaster::Attack(bool altAttack, int num_attacks, float spread, float fuseOffset, float power) {
	idVec3 muzzleOrigin;
	idMat3 muzzleAxis;

	if (!viewModel) {
		common->Warning("NULL viewmodel %s\n", __FUNCTION__);
		return;
	}

	if (viewModel->IsHidden()) {
		return;
	}

	// avoid all ammo considerations on an MP client
	if (!gameLocal.isClient) {
		// check if we're out of ammo or the clip is empty
		int ammoAvail = owner->inventory.HasAmmo(ammoType, ammoRequired);
		if (!ammoAvail || ((clipSize != 0) && (ammoClip <= 0))) {
			return;
		}

		owner->inventory.UseAmmo(ammoType, ammoRequired);
		if (clipSize && ammoRequired) {
			clipPredictTime = gameLocal.time;	// mp client: we predict this. mark time so we're not confused by snapshots
			ammoClip -= 1;
		}

		// wake up nearby monsters
		if (!wfl.silent_fire) {
			gameLocal.AlertAI(owner);
		}
	}

	// set the shader parm to the time of last projectile firing,
	// which the gun material shaders can reference for single shot barrel glows, etc
	viewModel->SetShaderParm(SHADERPARM_DIVERSITY, gameLocal.random.CRandomFloat());
	viewModel->SetShaderParm(SHADERPARM_TIMEOFFSET, -MS2SEC(gameLocal.realClientTime));

	if (worldModel.GetEntity()) {
		worldModel->SetShaderParm(SHADERPARM_DIVERSITY, viewModel->GetRenderEntity()->shaderParms[SHADERPARM_DIVERSITY]);
		worldModel->SetShaderParm(SHADERPARM_TIMEOFFSET, viewModel->GetRenderEntity()->shaderParms[SHADERPARM_TIMEOFFSET]);
	}

	// calculate the muzzle position
	if (barrelJointView != INVALID_JOINT && spawnArgs.GetBool("launchFromBarrel")) {
		// there is an explicit joint for the muzzle
		GetGlobalJointTransform(true, barrelJointView, muzzleOrigin, muzzleAxis);
	}
	else {
		// go straight out of the view
		muzzleOrigin = playerViewOrigin;
		muzzleAxis = playerViewAxis;
		muzzleOrigin += playerViewAxis[0] * muzzleOffset;
	}

	// add some to the kick time, incrementally moving repeat firing weapons back
	if (kick_endtime < gameLocal.realClientTime) {
		kick_endtime = gameLocal.realClientTime;
	}
	kick_endtime += muzzle_kick_time;
	if (kick_endtime > gameLocal.realClientTime + muzzle_kick_maxtime) {
		kick_endtime = gameLocal.realClientTime + muzzle_kick_maxtime;
	}

	// add the muzzleflash
	MuzzleFlash();

	// quad damage overlays a sound
	if (owner->PowerUpActive(POWERUP_QUADDAMAGE)) {
		viewModel->StartSound("snd_quaddamage", SND_CHANNEL_VOICE, 0, false, NULL);
	}

	// Muzzle flash effect
	bool muzzleTint = spawnArgs.GetBool("muzzleTint");
	viewModel->PlayEffect("fx_muzzleflash", flashJointView, false, vec3_origin, false, EC_IGNORE, muzzleTint ? owner->GetHitscanTint() : vec4_one);

	if (worldModel && flashJointWorld != INVALID_JOINT) {
		worldModel->PlayEffect(gameLocal.GetEffect(weaponDef->dict, "fx_muzzleflash_world"), flashJointWorld, vec3_origin, mat3_identity, false, vec3_origin, false, EC_IGNORE, muzzleTint ? owner->GetHitscanTint() : vec4_one);
	}

	owner->WeaponFireFeedback(&weaponDef->dict);

	// Inform the gui of the ammo change
	viewModel->PostGUIEvent("weapon_ammo");
	if (ammoClip == 0 && AmmoAvailable() == 0) {
		viewModel->PostGUIEvent("weapon_noammo");
	}

	if (gameLocal.towerManager->buildMode) {
		idVec3 hitPos = Tower_Raycast(altAttack ? attackAltDict : attackDict, muzzleOrigin, muzzleAxis, num_attacks, spread, power);

		gameLocal.towerManager->BuildTower(hitPos);
		return;
	}

	// The attack is either a hitscan or a launched projectile, do that now.
	if (!gameLocal.isClient) {
		idDict& dict = altAttack ? attackAltDict : attackDict;
		power *= owner->PowerUpModifier(PMOD_PROJECTILE_DAMAGE);
		if (altAttack ? wfl.attackAltHitscan : wfl.attackHitscan) {
			Hitscan(dict, muzzleOrigin, muzzleAxis, num_attacks, spread, power);
		}
		else {
			LaunchProjectiles(dict, muzzleOrigin, muzzleAxis, num_attacks, spread, fuseOffset, power);
		}
		//asalmon:  changed to keep stats even in single player 
		statManager->WeaponFired(owner, weaponIndex, num_attacks);

	}
}

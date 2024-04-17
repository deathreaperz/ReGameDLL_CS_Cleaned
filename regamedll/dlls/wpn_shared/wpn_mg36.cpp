//new weapon settings
#include "precompiled.h"

LINK_ENTITY_TO_CLASS(weapon_mg36, CMG36, CCSMG36)

void CMG36::Spawn()
{
	Precache();

	m_iId = WEAPON_MG36;
	SET_MODEL(edict(), "models/w_mg36.mdl");

	m_iDefaultAmmo = MG36_DEFAULT_GIVE;
	m_flAccuracy = 0.22f;
	m_iShotsFired = 0;

#ifdef REGAMEDLL_API
	CSPlayerWeapon()->m_flBaseDamage = MG36_DAMAGE;
#endif

	// Get ready to fall down
	FallInit();

	// extend
	CBasePlayerWeapon::Spawn();
}

void CMG36::Precache()
{
	PRECACHE_MODEL("models/v_mg36.mdl");
	PRECACHE_MODEL("models/w_mg36.mdl");

	PRECACHE_SOUND("weapons/mg36-1.wav");
	PRECACHE_SOUND("weapons/mg36_boltfull.wav");
	PRECACHE_SOUND("weapons/mg36_clipin.wav");
	PRECACHE_SOUND("weapons/mg36_clipout.wav");
	PRECACHE_SOUND("weapons/mg36_draw.wav");

	m_iShell = PRECACHE_MODEL("models/rshell.mdl");
	m_usFireMG36 = PRECACHE_EVENT(1, "events/mg36.sc");
}

int CMG36::GetItemInfo(ItemInfo* p)
{
	p->pszName = STRING(pev->classname);
	p->pszAmmo1 = "556NatoBox";
	p->iMaxAmmo1 = MAX_AMMO_556NATOBOX;
	p->pszAmmo2 = nullptr;
	p->iMaxAmmo2 = -1;
	p->iMaxClip = MG36_MAX_CLIP;
	p->iSlot = 0; //primary
	p->iPosition = 20; //alphabetically based on iSlot Category for all weapons
	p->iId = m_iId = WEAPON_MG36; //MAIN PROBLEM IS HERE, WHEN CHANGED TO WEAPON_SG552, IT WORKS BUT THE PRECACHED SOUND AND SPRITES FOLLOWS SG552'S, ALSO CAN'T RELOAD
	p->iFlags = 0; //special conditions
	p->iWeight = MG36_WEIGHT;

	return 1;
}

BOOL CMG36::Deploy()
{
	m_flAccuracy = 0.22f;
	m_iShotsFired = 0;
	iShellOn = 1;

	return DefaultDeploy("models/v_mg36.mdl", "models/p_mg36.mdl", MG36_DRAW, "m249", UseDecrement() != FALSE);
	//(first person model, player model, first person sequence, player sequence, use decrement)
}

void CMG36::SecondaryAttack()
{
	if (m_pPlayer->m_iFOV == DEFAULT_FOV)
		m_pPlayer->pev->fov = m_pPlayer->m_iFOV = 55;
	else
		m_pPlayer->pev->fov = m_pPlayer->m_iFOV = 90;

	m_flNextSecondaryAttack = UTIL_WeaponTimeBase() + 0.3f;
}

void CMG36::PrimaryAttack()
{
	const float flCycleTime = (m_pPlayer->pev->fov == DEFAULT_FOV) ? 0.0825f : 0.135f;

	if (!(m_pPlayer->pev->flags & FL_ONGROUND))
	{
		MG36Fire(0.05 + (0.42 * m_flAccuracy), flCycleTime, FALSE);
	}
	else if (m_pPlayer->pev->velocity.Length2D() > 140)
	{
		MG36Fire(0.05 + (0.037 * m_flAccuracy), flCycleTime, FALSE);
	}
	else
	{
		MG36Fire(0.037 * m_flAccuracy, flCycleTime, FALSE);
	}
}

void CMG36::MG36Fire(float flSpread, float flCycleTime, BOOL fUseAutoAim)
{
	Vector vecAiming, vecSrc, vecDir;
	int flag;

	m_bDelayFire = true;
	m_iShotsFired++;

	m_flAccuracy = ((m_iShotsFired * m_iShotsFired * m_iShotsFired) / MG36_ACCURACY_DIVISOR) + 0.3f;

	if (m_flAccuracy > 1.0f)
		m_flAccuracy = 1.0f;

	if (m_iClip <= 0)
	{
		if (m_fFireOnEmpty)
		{
			PlayEmptySound();
			m_flNextPrimaryAttack = GetNextAttackDelay(0.2);
		}

		if (TheBots)
		{
			TheBots->OnEvent(EVENT_WEAPON_FIRED_ON_EMPTY, m_pPlayer);
		}

		return;
	}

	m_iClip--;
	m_pPlayer->pev->effects |= EF_MUZZLEFLASH;
	m_pPlayer->SetAnimation(PLAYER_ATTACK1);

	m_pPlayer->m_iWeaponVolume = NORMAL_GUN_VOLUME;
	m_pPlayer->m_iWeaponFlash = BRIGHT_GUN_FLASH;

	UTIL_MakeVectors(m_pPlayer->pev->v_angle + m_pPlayer->pev->punchangle);

	vecSrc = m_pPlayer->GetGunPosition();
	vecAiming = gpGlobals->v_forward;

#ifdef REGAMEDLL_API
	float flBaseDamage = CSPlayerWeapon()->m_flBaseDamage;
#else
	float flBaseDamage = MG36_DAMAGE;
#endif
	vecDir = m_pPlayer->FireBullets3(vecSrc, vecAiming, flSpread, 8192, 2, BULLET_PLAYER_556MM,
		flBaseDamage, MG36_RANGE_MODIFER, m_pPlayer->pev, false, m_pPlayer->random_seed);

#ifdef CLIENT_WEAPONS
	flag = FEV_NOTHOST;
#else
	flag = 0;
#endif

	PLAYBACK_EVENT_FULL(flag, m_pPlayer->edict(), m_usFireMG36, 0, (float*)&g_vecZero, (float*)&g_vecZero, vecDir.x, vecDir.y,
		int(m_pPlayer->pev->punchangle.x * 100), int(m_pPlayer->pev->punchangle.y * 100), 5, FALSE);

	m_flNextPrimaryAttack = m_flNextSecondaryAttack = GetNextAttackDelay(flCycleTime);

	if (!m_iClip && m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType] <= 0)
	{
		m_pPlayer->SetSuitUpdate("!HEV_AMO0", SUIT_SENTENCE, SUIT_REPEAT_OK);
	}

	m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 2.0f;

	if (m_pPlayer->pev->velocity.Length2D() > 0)
	{
		KickBack(1.4, 0.3, 0.065, 0.04, 3.5, 2.0, 6); //walking
	}
	else if (!(m_pPlayer->pev->flags & FL_ONGROUND))
	{
		KickBack(1.1, 0.2, 0.04, 0.22, 2.5, 1.3, 8); //in ground / standing?
	}
	else if (m_pPlayer->pev->flags & FL_DUCKING)
	{
		KickBack(0.6, 0.15, 0.15, 0.01, 2.0, 0.8, 15); //ducking / crouching
	}
	else
	{
		KickBack(1.6, 0.6, 0.4, 0.125, 5.0, 3.0, 8); //in air?
	}
}

void CMG36::Reload()
{
	if (m_pPlayer->ammo_556natobox <= 0)
		return;

	if (DefaultReload(iMaxClip(), MG36_RELOAD, MG36_RELOAD_TIME))
	{
		if (m_pPlayer->m_iFOV != DEFAULT_FOV)
		{
			SecondaryAttack();
		}

		m_pPlayer->SetAnimation(PLAYER_RELOAD);
		m_flAccuracy = 0.22f;
		m_iShotsFired = 0;
		m_bDelayFire = false;
	}
}

void CMG36::WeaponIdle()
{
	ResetEmptySound();
	m_pPlayer->GetAutoaimVector(AUTOAIM_10DEGREES);

	if (m_flTimeWeaponIdle > UTIL_WeaponTimeBase())
	{
		return;
	}

	m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 20.0f;
	SendWeaponAnim(MG36_IDLE1, UseDecrement() != FALSE);
}

float CMG36::GetMaxSpeed()
{
	if (m_pPlayer->m_iFOV == DEFAULT_FOV)
		return MG36_MAX_SPEED;

	return MG36_MAX_SPEED_ZOOM;
}

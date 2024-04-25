#include "precompiled.h"

LINK_ENTITY_TO_CLASS(weapon_m60, CM60, CCSM60)

void CM60::Spawn()
{
	Precache();

	m_iId = WEAPON_M60;
	SET_MODEL(edict(), "models/w_m60.mdl");

	m_iDefaultAmmo = M60_DEFAULT_GIVE;
	m_flAccuracy = 0.25f;
	m_iShotsFired = 0;

#ifdef REGAMEDLL_API
	CSPlayerWeapon()->m_flBaseDamage = M60_DAMAGE;
#endif

	// Get ready to fall down
	FallInit();

	// extend
	CBasePlayerWeapon::Spawn();
}

void CM60::Precache()
{
	PRECACHE_MODEL("models/v_m60.mdl");
	PRECACHE_MODEL("models/w_m60.mdl");

	PRECACHE_SOUND("weapons/m60-1.wav");
	PRECACHE_SOUND("weapons/m60-2.wav");
	PRECACHE_SOUND("weapons/m60_boxout.wav");
	PRECACHE_SOUND("weapons/m60_boxin.wav");
	PRECACHE_SOUND("weapons/m60_chain.wav");
	PRECACHE_SOUND("weapons/m60_coverup.wav");
	PRECACHE_SOUND("weapons/m60_coverdown.wav");

	m_iShell = PRECACHE_MODEL("models/rshell.mdl");
	m_usFireM60 = PRECACHE_EVENT(1, "events/m60.sc");
}

int CM60::GetItemInfo(ItemInfo* p)
{
	p->pszName = STRING(pev->classname);
	p->pszAmmo1 = "556NatoBox";
	p->iMaxAmmo1 = MAX_AMMO_556NATOBOX;
	p->pszAmmo2 = nullptr;
	p->iMaxAmmo2 = -1;
	p->iMaxClip = M60_MAX_CLIP;
	p->iSlot = 0;
	p->iPosition = 20;
	p->iId = m_iId = WEAPON_M60;
	p->iFlags = 0;
	p->iWeight = M60_WEIGHT;

	return 1;
}

BOOL CM60::Deploy()
{
	m_flAccuracy = 0.25f;
	m_iShotsFired = 0;
	iShellOn = 1;

	return DefaultDeploy("models/v_m60.mdl", "models/p_m60.mdl", M60_DRAW, "m249", UseDecrement() != FALSE);
}

void CM60::PrimaryAttack()
{
	if (!(m_pPlayer->pev->flags & FL_ONGROUND))
	{
		M60Fire(0.05 + (0.2 * m_flAccuracy), 0.1, FALSE);
	}
	else if (m_pPlayer->pev->velocity.Length2D() > 140)
	{
		M60Fire(0.05 + (0.03 * m_flAccuracy), 0.1, FALSE);
	}
	else
	{
		M60Fire(0.03 * m_flAccuracy, 0.1, FALSE);
	}
}

void CM60::M60Fire(float flSpread, float flCycleTime, BOOL fUseAutoAim)
{
	Vector vecAiming, vecSrc, vecDir;
	int flag;

	m_bDelayFire = true;
	m_iShotsFired++;

	m_flAccuracy = ((m_iShotsFired * m_iShotsFired * m_iShotsFired) / M60_ACCURACY_DIVISOR) + 0.3f;

	if (m_flAccuracy > 0.9f)
		m_flAccuracy = 0.9f;

	if (m_iClip <= 0)
	{
		if (m_fFireOnEmpty)
		{
			PlayEmptySound();
			m_flNextPrimaryAttack = GetNextAttackDelay(0.1149);
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
	SendWeaponAnim(M60_SHOOT1, UseDecrement() != FALSE);

	UTIL_MakeVectors(m_pPlayer->pev->v_angle + m_pPlayer->pev->punchangle);

	m_pPlayer->m_iWeaponVolume = NORMAL_GUN_VOLUME;
	m_pPlayer->m_iWeaponFlash = BRIGHT_GUN_FLASH;
	EMIT_SOUND(ENT(pev), CHAN_VOICE, "weapons/m60-1.wav", VOL_NORM, ATTN_NORM);

	vecSrc = m_pPlayer->GetGunPosition();
	vecAiming = gpGlobals->v_forward;

#ifdef REGAMEDLL_API
	float flBaseDamage = CSPlayerWeapon()->m_flBaseDamage;
#else
	float flBaseDamage = M60_DAMAGE;
#endif
	vecDir = m_pPlayer->FireBullets3(vecSrc, vecAiming, flSpread, 8192, 6, BULLET_PLAYER_556MM,
		flBaseDamage, M60_RANGE_MODIFER, m_pPlayer->pev, false, m_pPlayer->random_seed);

#ifdef CLIENT_WEAPONS
	flag = FEV_NOTHOST;
#else
	flag = 0;
#endif

	PLAYBACK_EVENT_FULL(flag, m_pPlayer->edict(), m_usFireM60, 0, (float*)&g_vecZero, (float*)&g_vecZero, vecDir.x, vecDir.y,
		int(m_pPlayer->pev->punchangle.x * 100), int(m_pPlayer->pev->punchangle.y * 100), FALSE, FALSE);

	m_flNextPrimaryAttack = m_flNextSecondaryAttack = GetNextAttackDelay(flCycleTime);

	if (!m_iClip && m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType] <= 0)
	{
		m_pPlayer->SetSuitUpdate("!HEV_AMO0", SUIT_SENTENCE, SUIT_REPEAT_OK);
	}

	m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 2.0f / 16.0f;

	if (!(m_pPlayer->pev->flags & FL_ONGROUND))
	{
		KickBack(1.5, 0.55, 0.3, 0.3, 5.0, 5.0, 5);
	}
	else if (m_pPlayer->pev->velocity.Length2D() > 0)
	{
		KickBack(1.1, 0.3, 0.2, 0.06, 4.0, 3.0, 8);
	}
	else if (m_pPlayer->pev->flags & FL_DUCKING)
	{
		KickBack(0.75, 0.1, 0.1, 0.018, 3.5, 1.8, 9);
	}
	else
	{
		KickBack(0.9, 0.25, 0.2, 0.02, 3.2, 2.5, 7);
	}
}

void CM60::Reload()
{
#ifdef REGAMEDLL_FIXES
	// to prevent reload if not enough ammo
	if (m_pPlayer->ammo_556natobox <= 0)
		return;
#endif

	if (DefaultReload(iMaxClip(), M60_RELOAD, M60_RELOAD_TIME))
	{
		m_pPlayer->SetAnimation(PLAYER_RELOAD);

		m_flAccuracy = 0.25f;
		m_bDelayFire = false;
		m_iShotsFired = 0;

		return ReloadSound();
	}
}

void CM60::WeaponIdle()
{
	ResetEmptySound();
	m_pPlayer->GetAutoaimVector(AUTOAIM_10DEGREES);

	if (m_flTimeWeaponIdle > UTIL_WeaponTimeBase())
	{
		return;
	}

	m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 2.0f / 16.0f;
	SendWeaponAnim(M60_IDLE1, UseDecrement() != FALSE);
}

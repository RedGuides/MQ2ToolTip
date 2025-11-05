#include <mq/Plugin.h>
#include <chrono>

PreSetup("MQ2ToolTip");
PLUGIN_VERSION(1.0);
#define PLUGIN_NAME "MQ2ToolTip"

int oldX = 0, oldY = 0;
bool MouseHover = false;
bool gEnabled = false;
bool gFollowMouse = false;
bool gAutoClear = false;
unsigned int gClearTimer = 0;
bool gGuildOn = false;
uint64_t uClearTimerOld = 0;
CXRect gOldLocation = { 0, 0, 0, 0 };
char szToolTipINISection[MAX_STRING] = { 0 };
MQMouseInfo* pMousePos = EQADDR_MOUSE;
PlayerClient* pOldPlayer = nullptr;

using std::chrono::steady_clock;

constexpr std::chrono::milliseconds TooltipDelay{ 500 };
static steady_clock::time_point s_lastUpdate;

class CToolTipWnd : public CSidlScreenWnd
{
public:
	CToolTipWnd(const char* screenpiece)
		: CSidlScreenWnd(nullptr, screenpiece, -1, 1, nullptr)
	{
		CreateChildrenFromSidl();
		SetEscapable(false);
		SetWindowStyle((GetWindowStyle() & ~CWS_TITLE) | CWS_CLIENTMOVABLE);

		LoadWindowSettings();

		m_display = static_cast<CStmlWnd*>(GetChildItem("TT_Display"));
	}

	virtual ~CToolTipWnd() override
	{
		SaveWindowSettings();
	}

	void Update()
	{
		PlayerClient* pPlayer = pEverQuest->ClickedPlayer(pMousePos->X, pMousePos->Y);
		bool bRender = false;

		if (pPlayer && !*pMouseLook && MouseHover)
		{
			if (pPlayer->Rider)
				pPlayer = pPlayer->Rider;

			pOldPlayer = pPlayer;
			bRender = true;
			uClearTimerOld = GetTickCount64();
		}
		else if (pOldPlayer)
		{
			if (gFollowMouse)
			{
				if (GetTickCount64() > uClearTimerOld + gClearTimer && !IsMouseOver())
				{
					Show(false);
					return;
				}
			}

			if (!gAutoClear)
			{
				if (GetSpawnByID(pOldPlayer->SpawnID))
				{
					pPlayer = pOldPlayer;
					bRender = true;
				}
			}
			else if (GetTickCount64() > uClearTimerOld + gClearTimer)
			{
				m_display->SetSTMLText("", false);
				m_display->ForceParseNow();
			}
		}

		// GetPosition
		if (bRender)
		{
			if (pLocalPlayer)
			{
				char szTemp[MAX_STRING] = {};
				sprintf_s(szTemp, "%s\n%i %s", pPlayer->DisplayedName, pPlayer->Level,
					pEverQuest->GetClassThreeLetterCode(pPlayer->mActorClient.Class));

				if (bRender)
				{
					Show(false);

					if (gFollowMouse)
					{
						CXPoint pt;
						pt.x = pMousePos->X + 5;
						pt.y = pMousePos->Y + 5;
						Move(pt);
					}
				}

				unsigned int argbcolor = ConColor(pPlayer);
				unsigned int realcolor = ConColorToARGB(argbcolor) & 0x00FFFFFF;

				if (pTarget && pTarget->SpawnID == pPlayer->SpawnID)
				{
					sprintf_s(szTemp, "<c \"#%06X\">%s </c><c \"#00FFFF\">*TARGET*</c>", realcolor, pPlayer->DisplayedName);
				}
				else {
					sprintf_s(szTemp, "<c \"#%06X\">%s </c>", realcolor, pPlayer->DisplayedName);
				}

				m_display->SetSTMLText(szTemp, false);

				unsigned int typecolor = GetSpawnType(pPlayer);
				const char* sthetype = GetTypeDesc(GetSpawnType(pPlayer));
				sprintf_s(szTemp, "<BR>Level %i %s <c \"#%06X\"><%s></c>", pPlayer->Level,
					GetClassDesc(pPlayer->mActorClient.Class), typecolor, sthetype);

				if (gGuildOn)
				{
					if (const char* szGuild = GetGuildByID(pPlayer->GuildID))
					{
						sprintf_s(szTemp, "<BR><c \"#00FF00\">GUILD:</c>%s<BR>Level %i %s <c \"#%06X\"><%s></c>",
							szGuild, pPlayer->Level, GetClassDesc(pPlayer->mActorClient.Class), typecolor, sthetype);
					}
				}

				m_display->AppendSTML(szTemp);
				int64_t maxhp = pLocalPlayer->HPMax;
				int64_t hppct = 0;

				if (maxhp != 0)
				{
					hppct = pLocalPlayer->HPCurrent * 100 / maxhp;
				}

				int maxmana = pLocalPlayer->GetMaxMana();
				unsigned int manapct = 0;

				if (maxmana != 0)
				{
					manapct = pLocalPlayer->GetCurrentMana() * 100 / maxmana;
				}

				sprintf_s(szTemp, "<BR>ID: %i <c \"#FF69B4\">HP:</c> %I64d%% <c \"#0080FF\">Mana:</c> %d%%<BR>Loc: %.2f, %.2f, %.2f<br>Distance: %.2f",
					pPlayer->SpawnID, hppct, manapct, pPlayer->X, pPlayer->Y, pPlayer->Z,
					GetDistance3D(pLocalPlayer->X, pLocalPlayer->Y, pLocalPlayer->Z, pPlayer->X, pPlayer->Y, pPlayer->Z));

				m_display->AppendSTML(szTemp);
				m_display->ForceParseNow();

				if (bRender)
				{
					Show(true);
				}
			}
		}
	}

	void LoadWindowSettings()
	{
		strcpy_s(szToolTipINISection, "Default");

		SetLocation({
			GetPrivateProfileInt(szToolTipINISection, "WindowLeft",      200, INIFileName),
			GetPrivateProfileInt(szToolTipINISection, "WindowTop",       100, INIFileName),
			GetPrivateProfileInt(szToolTipINISection, "WindowRight",     400, INIFileName),
			GetPrivateProfileInt(szToolTipINISection, "WindowBottom",    200, INIFileName)
		});

		gOldLocation = GetLocation();
		SetLocked(GetPrivateProfileBool(szToolTipINISection, "Locked", false, INIFileName));
		SetFades(GetPrivateProfileBool(szToolTipINISection, "Fades", true, INIFileName));

		SetFadeDelay(GetPrivateProfileInt(szToolTipINISection, "Delay", 2000, INIFileName));
		SetFadeDuration(GetPrivateProfileInt(szToolTipINISection, "Duration", 500, INIFileName));
		SetAlpha(static_cast<uint8_t>(GetPrivateProfileInt(szToolTipINISection, "Alpha", 200, INIFileName)));
		SetFadeToAlpha(static_cast<uint8_t>(GetPrivateProfileInt(szToolTipINISection, "FadeToAlpha", 255, INIFileName)));
		SetBGType(GetPrivateProfileInt(szToolTipINISection, "BGType", 1, INIFileName));

		ARGBCOLOR col;
		col.A = static_cast<uint8_t>(GetPrivateProfileInt(szToolTipINISection, "BGTint.alpha", 255, INIFileName));
		col.R = static_cast<uint8_t>(GetPrivateProfileInt(szToolTipINISection, "BGTint.red", 0, INIFileName));
		col.G = static_cast<uint8_t>(GetPrivateProfileInt(szToolTipINISection, "BGTint.green", 0, INIFileName));
		col.B = static_cast<uint8_t>(GetPrivateProfileInt(szToolTipINISection, "BGTint.blue", 0, INIFileName));
		SetBGColor(col.ARGB);

		gFollowMouse = GetPrivateProfileBool(szToolTipINISection, "FollowMouse", false, INIFileName);
		gEnabled = GetPrivateProfileBool(szToolTipINISection, "Enabled", true, INIFileName);
		gAutoClear = GetPrivateProfileBool(szToolTipINISection, "AutoClear", true, INIFileName);
		gGuildOn = GetPrivateProfileBool(szToolTipINISection, "GuildOn", true, INIFileName);

		gClearTimer = GetPrivateProfileInt(szToolTipINISection, "ClearTimer", 1000, INIFileName);
	}

	void SaveWindowSettings()
	{
		strcpy_s(szToolTipINISection, "Default");

		WritePrivateProfileInt(szToolTipINISection, "WindowTop", GetLocation().top, INIFileName);
		WritePrivateProfileInt(szToolTipINISection, "WindowLeft", GetLocation().left, INIFileName);
		WritePrivateProfileInt(szToolTipINISection, "WindowRight", GetLocation().right, INIFileName);
		WritePrivateProfileInt(szToolTipINISection, "WindowBottom", GetLocation().bottom, INIFileName);

		WritePrivateProfileBool(szToolTipINISection, "Locked", IsLocked(), INIFileName);
		WritePrivateProfileBool(szToolTipINISection, "Fades", GetFades(), INIFileName);
		WritePrivateProfileInt(szToolTipINISection, "Delay", GetFadeDelay(), INIFileName);
		WritePrivateProfileInt(szToolTipINISection, "Duration", GetFadeDuration(), INIFileName);
		WritePrivateProfileInt(szToolTipINISection, "Alpha", GetAlpha(), INIFileName);
		WritePrivateProfileInt(szToolTipINISection, "FadeToAlpha", GetFadeToAlpha(), INIFileName);
		WritePrivateProfileInt(szToolTipINISection, "BGType", GetBGType(), INIFileName);

		ARGBCOLOR col;
		col.ARGB = GetBGColor();
		WritePrivateProfileInt(szToolTipINISection, "BGTint.alpha", col.A, INIFileName);
		WritePrivateProfileInt(szToolTipINISection, "BGTint.red", col.R, INIFileName);
		WritePrivateProfileInt(szToolTipINISection, "BGTint.green", col.G, INIFileName);
		WritePrivateProfileInt(szToolTipINISection, "BGTint.blue", col.B, INIFileName);

		//Plugin Options
		WritePrivateProfileBool(szToolTipINISection, "FollowMouse", gFollowMouse, INIFileName);
		WritePrivateProfileBool(szToolTipINISection, "Enabled", gEnabled, INIFileName);
		WritePrivateProfileBool(szToolTipINISection, "AutoClear", gAutoClear, INIFileName);
		WritePrivateProfileInt(szToolTipINISection, "ClearTimer", gClearTimer, INIFileName);
		WritePrivateProfileBool(szToolTipINISection, "GuildOn", gGuildOn, INIFileName);
	}

private:
	CStmlWnd* m_display = nullptr;
};

CToolTipWnd* pToolTipWnd = nullptr;

void DestroyToolTipWindow()
{
	if (pToolTipWnd)
	{
		delete pToolTipWnd;
		pToolTipWnd = nullptr;
	}
}

void CreateToolTipWindow()
{
	if (pToolTipWnd)
		return;

	DebugSpewAlways("%s::CreateToolTipWindow()", PLUGIN_NAME);

	if (pSidlMgr->FindScreenPieceTemplate("ToolTipWnd"))
	{
		pToolTipWnd = new CToolTipWnd("ToolTipWnd");
	}
}

void ToolTipCmd(PlayerClient*, const char* arg)
{
	char szArg[MAX_STRING] = {};
	GetArg(szArg, arg, 1);
	bool bSaveStuff = false;

	if (ci_equals(szArg, "on"))
	{
		gEnabled = true;
		bSaveStuff = true;

		WritePrivateProfileBool("Default", "Enabled", true, INIFileName);
		WriteChatf("ToolTip is now: \agON");

		if (!pToolTipWnd)
			CreateToolTipWindow();

		if (pToolTipWnd)
			pToolTipWnd->Show(true);
	}
	else if (ci_equals(szArg, "off"))
	{
		gEnabled = false;
		bSaveStuff = true;

		if (pToolTipWnd)
			pToolTipWnd->Show(false);

		WritePrivateProfileBool("Default", "Enabled", false, INIFileName);
		WriteChatf("ToolTip is now: \arOFF");
	}
	else if (ci_equals(szArg, "autoclear"))
	{
		GetArg(szArg, arg, 2);

		if (ci_equals(szArg, "on"))
		{
			gAutoClear = true;
			bSaveStuff = true;

			WriteChatf("Autoclear is now: \agON");
		}
		else if (ci_equals(szArg, "off"))
		{
			gAutoClear = false;
			bSaveStuff = true;

			WriteChatf("Autoclear is now: \arOFF");
		}
		else
		{
			WriteChatf("Autoclear is curently %s", gAutoClear ? "\agON" : "\arOFF");
		}

	}
	else if (ci_equals(szArg, "cleartimer"))
	{
		GetArg(szArg, arg, 2);

		if (szArg[0] && IsNumber(&szArg[0]))
		{
			gClearTimer = atoi(szArg);
			WriteChatf("ToolTip autoclear time: \ag%dms", gClearTimer);
		}
		else
		{
			WriteChatf("ToolTip autoclear reset to default: \ag1000ms");
			gClearTimer = 1000;
		}

		bSaveStuff = true;
	}
	else if (ci_equals(szArg, "follow"))
	{
		GetArg(szArg, arg, 2);

		if (ci_equals(szArg, "on"))
		{
			gFollowMouse = true;
			bSaveStuff = true;

			WriteChatf("FollowMouse is now: \agON");
		}
		else if (ci_equals(szArg, "off"))
		{
			gFollowMouse = false;
			bSaveStuff = true;
			WriteChatf("FollowMouse is now: \arOFF");
		}
		else
		{
			WriteChatf("FollowMouse is curently %s", gFollowMouse ? "\agON" : "\arOFF");
		}
	}
	else if (ci_equals(szArg, "guild"))
	{
		GetArg(szArg, arg, 2);

		if (ci_equals(szArg, "on"))
		{
			gGuildOn = true;
			bSaveStuff = true;

			WriteChatf("Displaying Guild is now: \arON");
		}
		else if (ci_equals(szArg, "off"))
		{
			gGuildOn = false;
			bSaveStuff = true;

			WriteChatf("Displaying Guild is now: \arOFF");
		}
		else
		{
			WriteChatf("Displaying Guild is curently %s", gGuildOn ? "\agON" : "\arOFF");
		}
	}
	else
	{
		WriteChatf("[ToolTip Help] - make sure your hud is on either by F11 or by typing /hud always)");
		WriteChatf("\ay/tooltip\ax \agon|off\ax (Enables or disables the Tooltip window)");
		WriteChatf("\ay/tooltip\ax \agautoclear on|off\ax (Clears the tooltip window if you move the mouse)");
		WriteChatf("\ay/tooltip\ax \agcleartimer <ms>\ax (Sets the number of milliseconds tooltip shoud wait before if clears the window if autoclear is on");
		WriteChatf("\ay/tooltip\ax \agfollow on|off\ax (Will follow the mouse around)");
		WriteChatf("\ay/tooltip\ax \agguild on|off\ax (Will display Guild tag in the tooltip as well)");
	}

	if (bSaveStuff)
	{
		if (pToolTipWnd)
		{
			pToolTipWnd->SaveWindowSettings();
		}
	}
}

PLUGIN_API void InitializePlugin()
{
	AddXMLFile("MQUI_ToolTipWnd.xml");
	AddCommand("/tooltip", ToolTipCmd);
}

PLUGIN_API void ShutdownPlugin()
{
	RemoveCommand("/tooltip");
	DestroyToolTipWindow();
}

PLUGIN_API void OnPulse()
{
	if (gGameState != GAMESTATE_INGAME || !pLocalPlayer)
		return;

	if (!pToolTipWnd)
	{
		CreateToolTipWindow();
	}

	if (!gEnabled || !pToolTipWnd)
		return;

	if (oldX != pMousePos->X || oldY != pMousePos->Y)
	{
		MouseHover = false;
		oldX = pMousePos->X;
		oldY = pMousePos->Y;

		s_lastUpdate = steady_clock::now();
	}

	if (steady_clock::now() - s_lastUpdate > TooltipDelay)
	{
		MouseHover = true;
	}

	if (!pToolTipWnd->IsMouseOver())
	{
		if (pToolTipWnd->GetLocation() != gOldLocation)
		{
			gOldLocation = pToolTipWnd->GetLocation();
			pToolTipWnd->SaveWindowSettings();
		}
	}

	pToolTipWnd->Update();
}

PLUGIN_API void OnCleanUI()
{
	DestroyToolTipWindow();
}

// Called once directly after the game ui is reloaded, after issuing /loadskin
PLUGIN_API void OnReloadUI()
{
	if (gGameState == GAMESTATE_INGAME && pLocalPlayer)
	{
		CreateToolTipWindow();
	}
}

PLUGIN_API void OnRemoveSpawn(PlayerClient* pSpawn)
{
	if (pSpawn == pOldPlayer)
	{
		pOldPlayer = nullptr;
	}
}

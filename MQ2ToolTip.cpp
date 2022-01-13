#include <mq/Plugin.h>

PreSetup("MQ2ToolTip");
#define PLUGIN_NAME "MQ2ToolTip"

void SaveWindowSettings(CSidlScreenWnd* pWindow);
void LoadWindowSettings(CSidlScreenWnd* pWindow);
unsigned int oldX = 0, oldY = 0;
bool MouseHover = 0;
unsigned int Time = 0;
bool gEnabled = false;
bool gFollowMouse = false;
bool gAutoClear = false;
unsigned int gClearTimer = 0;
bool gGuildOn = false;
uint64_t uClearTimerOld = 0;
CXRect gOldLocation = { 0, 0, 0, 0 };
char szToolTipINISection[MAX_STRING] = { 0 };
MQMouseInfo* pMousePos = EQADDR_MOUSE;
constexpr int DelayMs = 500;

class CToolTipWnd;
CToolTipWnd* pToolTipWnd = nullptr;

class CToolTipWnd : public CSidlScreenWnd
{
public:
	CToolTipWnd(CXStr& screenpiece) : CSidlScreenWnd(nullptr, screenpiece, -1, 1, nullptr)
	{
		CreateChildrenFromSidl();
		//pXWnd()->Show(1,1);
		SetEscapable(false);
		Display = (CStmlWnd*)GetChildItem("TT_Display");
	}

	CToolTipWnd(char* screenpiece) : CSidlScreenWnd(nullptr, CXStr(screenpiece), -1, 1, nullptr)
	{
		CreateChildrenFromSidl();
		// pXWnd()->Show(1,1);
		SetEscapable(false);
		Display = (CStmlWnd*)GetChildItem("TT_Display");
	}

	~CToolTipWnd() {};

	CStmlWnd* Display = nullptr;
};

VOID DestroyToolTipWindow()
{
	if (pToolTipWnd)
	{
		SaveWindowSettings(pToolTipWnd);
		delete pToolTipWnd;
		pToolTipWnd = nullptr;
	}
}

void CreateToolTipWindow()
{
	try {
		if (pToolTipWnd)
			return;

		DebugSpewAlways("%s::CreateHelpWindow()", PLUGIN_NAME);

		if (pSidlMgr->FindScreenPieceTemplate("ToolTipWnd")) {
			if (pToolTipWnd = new CToolTipWnd("ToolTipWnd")) {
				LoadWindowSettings(pToolTipWnd);
			}
		}
	}
	catch (...) {
		MessageBox(NULL, "CRAP! in CreateToolTipWindow", "An exception occured", MB_OK);
	}
}

void ToolTipCmd(PSPAWNINFO pSpawn, char* arg)
{
	char szArg[MAX_STRING] = { 0 };
	GetArg(szArg, arg, 1);
	bool bSaveStuff = false;

	if (szArg[0]) {
		if (!_stricmp(szArg, "on")) {
			gEnabled = false;
			WritePrivateProfileStringA("Default", "Enabled", "1", INIFileName);
			WriteChatColor("ToolTip is now ON", CONCOLOR_LIGHTBLUE);

			if (!pToolTipWnd)
				CreateToolTipWindow();

			if (pToolTipWnd)
				pToolTipWnd->Show(1, 1);

			bSaveStuff = true;

		} else if (!_stricmp(szArg, "off")) {
			gEnabled = false;
			WritePrivateProfileStringA("Default", "Enabled", "0", INIFileName);
			WriteChatColor("ToolTip is now OFF"), CONCOLOR_YELLOW;
			if (pToolTipWnd)
				pToolTipWnd->Show(0, 1);

			bSaveStuff = true;

		} else if (!_stricmp(szArg, "autoclear")) {
			GetArg(szArg, arg, 2);
			if (!_stricmp(szArg, "on")) {
				gAutoClear = true;
				WriteChatColor("Autoclear is now ON", CONCOLOR_LIGHTBLUE);
				bSaveStuff = true;
			}

			if (!_stricmp(szArg, "off")) {
				gAutoClear = false;
				WriteChatColor("Autoclear is now OFF", CONCOLOR_YELLOW);
				bSaveStuff = true;
			}

			if (!bSaveStuff)
				WriteChatf("Autoclear is curently %s", gAutoClear ? "TRUE" : "FALSE");

		} else if (!_stricmp(szArg, "cleartimer")) {
			GetArg(szArg, arg, 2);
			if (szArg[0] && IsNumber(&szArg[0])) {
				gClearTimer = atoi(szArg);
				WriteChatf("ToolTip autoclear time: %d ms", gClearTimer);
			}
			else {
				WriteChatColor("ToolTip autoclear reset to default: 1000 ms", CONCOLOR_YELLOW);
				gClearTimer = 1000;
			}

			bSaveStuff = true;

		} else if (!_stricmp(szArg, "follow")) {
			GetArg(szArg, arg, 2);
			if (!_stricmp(szArg, "on")) {
				gFollowMouse = true;
				WriteChatColor("FollowMouse is now ON", CONCOLOR_LIGHTBLUE);
				bSaveStuff = true;
			}

			if (!_stricmp(szArg, "off")) {
				gFollowMouse = false;
				WriteChatColor("FollowMouse is now OFF", CONCOLOR_YELLOW);
				bSaveStuff = true;
			}

			if (!bSaveStuff)
				WriteChatf("FollowMouse is curently %s", gFollowMouse ? "TRUE" : "FALSE");

		} else if (!_stricmp(szArg, "guild")) {
			GetArg(szArg, arg, 2);
			if (!_stricmp(szArg, "on")) {
				gGuildOn = true;
				WriteChatColor("Displaying Guild is now ON", CONCOLOR_LIGHTBLUE);
				bSaveStuff = true;
			}

			if (!_stricmp(szArg, "off")) {
				gGuildOn = false;
				WriteChatColor("Displaying Guild is now OFF", CONCOLOR_YELLOW);
				bSaveStuff = true;
			}

			if (!bSaveStuff)
				WriteChatf("Displaying Guild is curently %s", gGuildOn ? "TRUE" : "FALSE");
		}

	}
	else {
		WriteChatColor("[ToolTip Help] - make sure your hud is on either by F11 or by typing /hud always)", CONCOLOR_YELLOW);
		WriteChatColor("/tooltip on|off {Enables or disables the Tooltip window)", CONCOLOR_YELLOW);
		WriteChatColor("/tooltip autoclear on|off (clears the tooltip window if you move the mouse)", CONCOLOR_YELLOW);
		WriteChatColor("/tooltip cleartimer <ms> (Sets the number of milliseconds tooltip shoud wait before if clears the window if autoclear is on", CONCOLOR_YELLOW);
		WriteChatColor("/tooltip follow on|off (Will follow the mouse around)", CONCOLOR_YELLOW);
		WriteChatColor("/tooltip guild on|off (Will display Guild tag in the tooltip as well)", CONCOLOR_YELLOW);
	}

	if (bSaveStuff)
		SaveWindowSettings(pToolTipWnd);

	return;
}

PLUGIN_API VOID InitializePlugin()
{
	DebugSpewAlways("Initializing MQ2ToolTip");
	AddXMLFile("MQUI_ToolTipWnd.xml");
	AddCommand("/tooltip", ToolTipCmd, FALSE, TRUE);
}

PLUGIN_API VOID ShutdownPlugin()
{
	DebugSpewAlways("Shutting down MQ2ToolTip");
	RemoveCommand("/tooltip");
	DestroyToolTipWindow();
}

PlayerClient* pOldPlayer = 0;
PLUGIN_API VOID OnDrawHUD()
{
	if (gEnabled && gGameState == GAMESTATE_INGAME) {
		PlayerClient* pPlayer = pEverQuest->ClickedPlayer(pMousePos->X, pMousePos->Y);
		bool bRender = false;
		if (pPlayer && !*pMouseLook && MouseHover) {
			if (pPlayer->Rider)
				pPlayer = pPlayer->Rider;

			pOldPlayer = pPlayer;
			bRender = true;
			uClearTimerOld = GetTickCount64();
		}
		else if (pOldPlayer) {
			if (gFollowMouse && pToolTipWnd) {
				if (GetTickCount64() > uClearTimerOld + gClearTimer && !pToolTipWnd->IsMouseOver()) {
					pToolTipWnd->Show(0, 1);
					return;
				}
			}

			if (!gAutoClear) {
				if (GetSpawnByID(pOldPlayer->SpawnID)) {
					pPlayer = pOldPlayer;
					bRender = true;
				}
			}
			else if (GetTickCount64() > uClearTimerOld + gClearTimer && pToolTipWnd) {
				pToolTipWnd->Display->SetSTMLText("", 1);
				pToolTipWnd->Display->ForceParseNow();
			}
		}

		//GetPosition
		if (bRender && pToolTipWnd) {
			if (pLocalPC && pLocalPlayer) {
				char szTemp[MAX_STRING] = { 0 };
				sprintf_s(szTemp, "%s\n%i %s", pPlayer->DisplayedName, pPlayer->Level, pEverQuest->GetClassThreeLetterCode(pPlayer->mActorClient.Class));

				if (bRender) {
					pToolTipWnd->Show(false, true);

					if (gFollowMouse) {
						CXPoint pt;
						pt.x = pMousePos->X + 5;
						pt.y = pMousePos->Y + 5;
						pToolTipWnd->Move(pt);
					}
				}

				unsigned int argbcolor = ConColor(pPlayer);
				unsigned int realcolor = ConColorToARGB(argbcolor) & 0x00FFFFFF;

				if (pTarget && pTarget->SpawnID == pPlayer->SpawnID) {
					sprintf_s(szTemp, "<c \"#%06X\">%s </c><c \"#00FFFF\">*TARGET*</c>", realcolor, pPlayer->DisplayedName);
				}
				else {
					sprintf_s(szTemp, "<c \"#%06X\">%s </c>", realcolor, pPlayer->DisplayedName);
				}

				CXStr NewText(szTemp);
				pToolTipWnd->Display->SetSTMLText(NewText, 1, 0);
				unsigned int typecolor = GetSpawnType((SPAWNINFO*)pPlayer);
				const char* sthetype = GetTypeDesc(GetSpawnType(pPlayer));
				sprintf_s(szTemp, "<BR>Level %i %s <c \"#%06X\"><%s></c>", pPlayer->Level, GetClassDesc(pPlayer->mActorClient.Class), typecolor, sthetype);

				if (gGuildOn) {
					const char* szGuild = GetGuildByID(pPlayer->GuildID);
					if (const char* theguild = szGuild) {
						sprintf_s(szTemp, "<BR><c \"#00FF00\">GUILD:</c>%s<BR>Level %i %s <c \"#%06X\"><%s></c>", theguild, pPlayer->Level, GetClassDesc(pPlayer->mActorClient.Class), typecolor, sthetype);
					}
				}

				pToolTipWnd->Display->AppendSTML(szTemp);
				int64_t maxhp = pLocalPlayer->HPMax;
				int64_t hppct = 0;

				if (maxhp != 0) {
					hppct = pLocalPlayer->HPCurrent * 100 / maxhp;
				}

				int maxmana = pLocalPlayer->GetMaxMana();
				unsigned int manapct = 0;

				if (maxmana != 0) {
					manapct = pLocalPlayer->GetCurrentMana() * 100 / maxmana;
				}

				sprintf_s(szTemp, "<BR>ID: %i <c \"#FF69B4\">HP:</c> %I64d%% <c \"#0080FF\">Mana:</c> %d%%<BR>Loc: %.2f, %.2f, %.2f<br>Distance: %.2f", pPlayer->SpawnID, hppct, manapct, pPlayer->X, pPlayer->Y, pPlayer->Z, GetDistance3D(pLocalPlayer->X, pLocalPlayer->Y, pLocalPlayer->Z, pPlayer->X, pPlayer->Y, pPlayer->Z));
				pToolTipWnd->Display->AppendSTML(szTemp);
				pToolTipWnd->Display->ForceParseNow();

				if (bRender) {
					pToolTipWnd->Show(1, 1);
				}
			}
		}
	}
}

PLUGIN_API VOID OnPulse()
{
	if (gGameState != GAMESTATE_INGAME)
		return;

	if (gGameState == GAMESTATE_INGAME && pCharSpawn) {
		if (!pToolTipWnd) {
			CreateToolTipWindow();
		}
	}

	if (!gEnabled)
		return;

	if (PSPAWNINFO ps = GetCharInfo()->pSpawn) {
		if (oldX != pMousePos->X || oldY != pMousePos->Y) {
			MouseHover = false;
			oldX = pMousePos->X;
			oldY = pMousePos->Y;
			Time = ps->TimeStamp;
		}

		if (ps->TimeStamp - Time > DelayMs) {
			MouseHover = true;
		}
	}

	if (pToolTipWnd && !pToolTipWnd->IsMouseOver()) {
		if (pToolTipWnd->GetLocation() != gOldLocation) {
			gOldLocation = pToolTipWnd->GetLocation();
			SaveWindowSettings((CSidlScreenWnd*)pToolTipWnd);
		}
	}
}

VOID LoadWindowSettings(CSidlScreenWnd* pWindow)
{
	strcpy_s(szToolTipINISection, "Default");

	pWindow->SetLocation({ (LONG)GetPrivateProfileInt(szToolTipINISection,"WindowLeft",   200,INIFileName),
		(LONG)GetPrivateProfileInt(szToolTipINISection,"WindowTop",       100,INIFileName),
		(LONG)GetPrivateProfileInt(szToolTipINISection,"WindowRight",      400,INIFileName),
		(LONG)GetPrivateProfileInt(szToolTipINISection,"WindowBottom",    200,INIFileName)
	});

	gOldLocation = pWindow->GetLocation();
	pWindow->SetLocked(GetPrivateProfileBool(szToolTipINISection, "Locked", false, INIFileName));
	pWindow->SetFades(GetPrivateProfileBool(szToolTipINISection, "Fades", true, INIFileName));
	pWindow->SetFadeDelay(GetPrivateProfileInt(szToolTipINISection, "Delay", 2000, INIFileName));
	pWindow->SetFadeDuration(GetPrivateProfileInt(szToolTipINISection, "Duration", 500, INIFileName));
	pWindow->SetAlpha(GetPrivateProfileInt(szToolTipINISection, "Alpha", 200, INIFileName));
	pWindow->SetFadeToAlpha(GetPrivateProfileInt(szToolTipINISection, "FadeToAlpha", 255, INIFileName));
	pWindow->SetBGType(GetPrivateProfileInt(szToolTipINISection, "BGType", 1, INIFileName));

	ARGBCOLOR col = { 0 };
	col.A = GetPrivateProfileInt(szToolTipINISection, "BGTint.alpha", 255, INIFileName);
	col.R = GetPrivateProfileInt(szToolTipINISection, "BGTint.red", 0, INIFileName);
	col.G = GetPrivateProfileInt(szToolTipINISection, "BGTint.green", 0, INIFileName);
	col.B = GetPrivateProfileInt(szToolTipINISection, "BGTint.blue", 0, INIFileName);
	pWindow->SetBGColor(col.ARGB);

	gFollowMouse = GetPrivateProfileBool(szToolTipINISection, "FollowMouse", false, INIFileName);
	gEnabled = GetPrivateProfileBool(szToolTipINISection, "Enabled", true, INIFileName);
	gAutoClear = GetPrivateProfileBool(szToolTipINISection, "AutoClear", true, INIFileName);
	gClearTimer = GetPrivateProfileInt(szToolTipINISection, "ClearTimer", 1000, INIFileName);
	gGuildOn = GetPrivateProfileBool(szToolTipINISection, "GuildOn", true, INIFileName);
}

template <unsigned int _Size>LPSTR SafeItoa(int _Value, char(&_Buffer)[_Size], int _Radix)
{
	errno_t err = _itoa_s(_Value, _Buffer, _Radix);
	if (!err) {
		return _Buffer;
	}

	return "";
}

void SaveWindowSettings(CSidlScreenWnd* pWindow)
{
	if (!pWindow)
		return;

	char szTemp[MAX_STRING] = { 0 };
	strcpy_s(szToolTipINISection, "Default");

	WritePrivateProfileInt(szToolTipINISection, "WindowTop", pWindow->GetLocation().top, INIFileName);
	WritePrivateProfileInt(szToolTipINISection, "WindowLeft", pWindow->GetLocation().left, INIFileName);
	WritePrivateProfileInt(szToolTipINISection, "WindowRight", pWindow->GetLocation().right, INIFileName);
	WritePrivateProfileInt(szToolTipINISection, "WindowBottom", pWindow->GetLocation().bottom, INIFileName);

	WritePrivateProfileBool(szToolTipINISection, "Locked", pWindow->IsLocked(), INIFileName);
	WritePrivateProfileBool(szToolTipINISection, "Fades", pWindow->GetFades(), INIFileName);
	WritePrivateProfileInt(szToolTipINISection, "Delay", pWindow->GetFadeDelay(), INIFileName);
	WritePrivateProfileInt(szToolTipINISection, "Duration", pWindow->GetFadeDuration(), INIFileName);
	WritePrivateProfileInt(szToolTipINISection, "Alpha", pWindow->GetAlpha(), INIFileName);
	WritePrivateProfileInt(szToolTipINISection, "FadeToAlpha", pWindow->GetFadeToAlpha(), INIFileName);
	WritePrivateProfileInt(szToolTipINISection, "BGType", pWindow->GetBGType(), INIFileName);

	ARGBCOLOR col = { 0 };
	col.ARGB = pWindow->GetBGColor();
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

PLUGIN_API VOID OnCleanUI()
{
	DebugSpewAlways("MQ2ToolTip::OnCleanUI()");
	DestroyToolTipWindow();
}

// Called once directly after the game ui is reloaded, after issuing /loadskin
PLUGIN_API VOID OnReloadUI()
{
	DebugSpewAlways("MQ2ToolTip::OnReloadUI()");
	if (gGameState == GAMESTATE_INGAME && pLocalPlayer) {
		CreateToolTipWindow();
	}
}


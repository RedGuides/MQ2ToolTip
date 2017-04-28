#include "../MQ2Plugin.h"


PreSetup("MQ2ToolTip");
#define PLUGIN_NAME "MQ2ToolTip"

VOID SaveWindowSettings(PCSIDLWND pWindow);
VOID LoadWindowSettings(PCSIDLWND pWindow);
PCHAR GetSpawnTypeName(LPDWORD type);
DWORD oldX = 0, oldY = 0;
BOOL MouseHover = 0;
DWORD Time = 0;
BOOL gEnabled = FALSE;
BOOL gFollowMouse = FALSE;
BOOL gAutoClear = FALSE;
DWORD gClearTimer = 0;
BOOL gGuildOn = 0;
ULONGLONG uClearTimerOld = 0;
RECT gOldLocation = {0};
CHAR szToolTipINISection[MAX_STRING]={0};
#define pMousePos EQADDR_MOUSE

#define DelayMs 500

class CToolTipWnd;
CToolTipWnd *pToolTipWnd=0;

class CToolTipWnd : public CSidlScreenWnd
{
public:
    CToolTipWnd(CXStr *screenpiece):CSidlScreenWnd(0,screenpiece,-1,1,0)
    {
        CreateChildrenFromSidl();
        //pXWnd()->Show(1,1);
        ReplacevfTable();
        CloseOnESC=0;
    }

    CToolTipWnd(char *screenpiece):CSidlScreenWnd(0,&CXStr(screenpiece),-1,1,0)
    {
        CreateChildrenFromSidl();
       // pXWnd()->Show(1,1);
        ReplacevfTable();
        CloseOnESC=0;
		SetWndNotification(CToolTipWnd);
		Display=(CStmlWnd*)GetChildItem("TT_Display");
    }

    ~CToolTipWnd()
    {
        RemovevfTable();
    }
    int WndNotification(CXWnd *pWnd, unsigned int Message, void *unknown)
    {    
		return CSidlScreenWnd::WndNotification(pWnd,Message,unknown);
    }

    void ReplacevfTable()
    {
        OldvfTable=((_CSIDLWND*)this)->pvfTable;
        PCSIDLWNDVFTABLE NewvfTable=new CSIDLWNDVFTABLE;
        memcpy(NewvfTable,OldvfTable,sizeof(CSIDLWNDVFTABLE));
        ((_CSIDLWND*)this)->pvfTable=NewvfTable;
    }

    void RemovevfTable()
    {
        PCSIDLWNDVFTABLE NewvfTable=((_CSIDLWND*)this)->pvfTable;
        ((_CSIDLWND*)this)->pvfTable=OldvfTable;
        delete NewvfTable;
    }

    void SetvfTable(DWORD index, DWORD value)
    {
        DWORD* vtable=(DWORD*)((_CSIDLWND*)this)->pvfTable;
        vtable[index]=value;
    }
/*0x218*/   CStmlWnd   *Display;
/*0x22C*/	PCSIDLWNDVFTABLE OldvfTable;
};

VOID DestroyToolTipWindow() 
{ 
    if (pToolTipWnd) 
    { 
		SaveWindowSettings((PCSIDLWND)pToolTipWnd);
        delete pToolTipWnd; 
        pToolTipWnd=0;  
    } 
}

void CreateToolTipWindow() 
{
	try {
		if (pToolTipWnd) {
			return; 
		}
		DebugSpewAlways("%s::CreateHelpWindow()", PLUGIN_NAME); 
		if (pSidlMgr->FindScreenPieceTemplate("ToolTipWnd")) { 
			if(pToolTipWnd = new CToolTipWnd("ToolTipWnd")) {
				LoadWindowSettings((PCSIDLWND)pToolTipWnd);
			}
		}
	} 
	catch(...) { 
		MessageBox(NULL,"CRAP! in CreateToolTipWindow","An exception occured",MB_OK);
	}
}
VOID ToolTipCmd(PSPAWNINFO pSpawn,PCHAR arg)
{
	CHAR szArg1[MAX_STRING] = {0}; 
	CHAR szArg2[MAX_STRING] = {0}; 
    GetArg(szArg1, arg, 1);
	if(szArg1[0]) {
		if(!_stricmp(szArg1,"on")) {
			gEnabled = TRUE;
			WritePrivateProfileStringA("Default","Enabled","1",INIFileName);
			WriteChatColor("ToolTip is now ON",CONCOLOR_LIGHTBLUE);
			if(!pToolTipWnd) {
				CreateToolTipWindow();
			}
			if(pToolTipWnd) {
				((CXWnd *)pToolTipWnd)->Show(1,1);
			}
			goto savestuff;
		}
		if(!_stricmp(szArg1,"off")) {
			gEnabled = FALSE;
			WritePrivateProfileStringA("Default","Enabled","0",INIFileName);
			WriteChatColor("ToolTip is now OFF"),CONCOLOR_YELLOW;
			if(pToolTipWnd) {
				((CXWnd *)pToolTipWnd)->Show(0,1);
			}
			goto savestuff;;
		}
		if(!_stricmp(szArg1,"autoclear")) {
			GetArg(szArg2, arg, 2);
			if(!_stricmp(szArg2,"on")) {
				gAutoClear = TRUE;
				WriteChatColor("Autoclear is now ON",CONCOLOR_LIGHTBLUE);
				goto savestuff;;
			}
			if(!_stricmp(szArg2,"off")) {
				gAutoClear = FALSE;
				WriteChatColor("Autoclear is now OFF",CONCOLOR_YELLOW);
				goto savestuff;;
			}
			WriteChatf("Autoclear is curently %s",gAutoClear?"TRUE":"FALSE");
			return;
		}
		if(!_stricmp(szArg1,"cleartimer")) {
			GetArg(szArg2, arg, 2);
			if(szArg2[0] && IsNumber(&szArg2[0])) {
				gClearTimer = atoi(szArg2);
				WriteChatf("ToolTip autoclear time: %d ms",gClearTimer);
			} else {
				WriteChatColor("ToolTip autoclear reset to default: 1000 ms",CONCOLOR_YELLOW);
				gClearTimer = 1000;
			}
			goto savestuff;
		}
		if(!_stricmp(szArg1,"follow")) {
			GetArg(szArg2, arg, 2);
			if(!_stricmp(szArg2,"on")) {
				gFollowMouse = TRUE;
				WriteChatColor("FollowMouse is now ON",CONCOLOR_LIGHTBLUE);
				goto savestuff;
			}
			if(!_stricmp(szArg2,"off")) {
				gFollowMouse = FALSE;
				WriteChatColor("FollowMouse is now OFF",CONCOLOR_YELLOW);
				goto savestuff;
			}
			WriteChatf("FollowMouse is curently %s",gFollowMouse?"TRUE":"FALSE");
			return;
		}
		if(!_stricmp(szArg1,"guild")) {
			GetArg(szArg2, arg, 2);
			if(!_stricmp(szArg2,"on")) {
				gGuildOn = TRUE;
				WriteChatColor("Displaying Guild is now ON",CONCOLOR_LIGHTBLUE);
				goto savestuff;;
			}
			if(!_stricmp(szArg2,"off")) {
				gGuildOn = FALSE;
				WriteChatColor("Displaying Guild is now OFF",CONCOLOR_YELLOW);
				goto savestuff;;
			}
			WriteChatf("Displaying Guild is curently %s",gAutoClear?"TRUE":"FALSE");
			return;
		}

	} else {
		WriteChatColor("[ToolTip Help] - make sure your hud is on either by F11 or by typing /hud always)",CONCOLOR_YELLOW);
		WriteChatColor("/tooltip on|off {Enables or disables the Tooltip window)",CONCOLOR_YELLOW);
		WriteChatColor("/tooltip autoclear on|off (clears the tooltip window if you move the mouse)",CONCOLOR_YELLOW);
		WriteChatColor("/tooltip cleartimer <ms> (Sets the number of milliseconds tooltip shoud wait before if clears the window if autoclear is on",CONCOLOR_YELLOW);
		WriteChatColor("/tooltip follow on|off (Will follow the mouse around)",CONCOLOR_YELLOW);
		WriteChatColor("/tooltip guild on|off (Will display Guild tag in the tooltip as well)",CONCOLOR_YELLOW);
	}
savestuff:
	if(pToolTipWnd) {
		SaveWindowSettings((PCSIDLWND)pToolTipWnd);
	}
	return;
}
PLUGIN_API VOID InitializePlugin(VOID)
{   
	DebugSpewAlways("Initializing MQ2ToolTip");
	AddXMLFile("MQUI_ToolTipWnd.xml");
	AddCommand("/tooltip",ToolTipCmd,FALSE,TRUE);
}

PLUGIN_API VOID ShutdownPlugin(VOID)
{
   DebugSpewAlways("Shutting down MQ2ToolTip");
   RemoveCommand("/tooltip");
   DestroyToolTipWindow();
}
EQPlayer* pOldPlayer = 0;
PLUGIN_API VOID OnDrawHUD(VOID)
{
	if (gEnabled && gGameState==GAMESTATE_INGAME) {
		EQPlayer* pPlayer = pEverQuest->ClickedPlayer(pMousePos->X, pMousePos->Y);
		BOOL bRender = 0;
		if (pPlayer && !*pMouseLook && MouseHover) {
			if(pPlayer->Data.Rider)
				pPlayer=(EQPlayer*)pPlayer->Data.Rider;
			pOldPlayer = pPlayer;
			bRender = 1;
			uClearTimerOld = MQGetTickCount64();
		} else {
			if(pOldPlayer) {
				if(gFollowMouse && pToolTipWnd) {
					if(MQGetTickCount64()>uClearTimerOld+gClearTimer && !pToolTipWnd->MouseOver) {
						((CXWnd *)pToolTipWnd)->Show(0,1);
						return;
					}
				}
				if(gAutoClear==0) {
					if(GetSpawnByID(((PSPAWNINFO)pOldPlayer)->SpawnID)) {
						pPlayer=pOldPlayer;
						bRender = 2;
					}
				} else {
					if(MQGetTickCount64()>uClearTimerOld+gClearTimer) {
						if(pToolTipWnd) { 
							((CStmlWnd*)pToolTipWnd->Display)->SetSTMLText("",1,0);
							((CStmlWnd*)pToolTipWnd->Display)->ForceParseNow();
						}	
					}
				}
			}
		}
		//GetPosition
		if(bRender) {
			if(pToolTipWnd) {
				if(PCHARINFO pChar = GetCharInfo()) {
					if(PSPAWNINFO pSpawn = pChar->pSpawn) {
						CHAR szTemp[MAX_STRING] = {0};
						sprintf_s(szTemp, "%s\n%i %s", pPlayer->Data.DisplayedName, pPlayer->Data.Level, pEverQuest->GetClassThreeLetterCode(pPlayer->Data.mActorClient.Class));
						if(bRender==1) {
							((CXWnd *)pToolTipWnd)->Show(0,1);
							if(gFollowMouse) {
								CXPoint pt;
								pt.A = pMousePos->X+5;
								pt.B = pMousePos->Y+5;
								((CXWnd *)pToolTipWnd)->Move(pt);
							}
						}
						DWORD argbcolor = ConColor((PSPAWNINFO)pPlayer);
						DWORD realcolor = ConColorToARGB(argbcolor) & 0x00FFFFFF;
						if(pTarget && ((PSPAWNINFO)pTarget)->SpawnID==pPlayer->Data.SpawnID) {
							sprintf_s(szTemp,"<c \"#%06X\">%s </c><c \"#00FFFF\">*TARGET*</c>",realcolor,pPlayer->Data.DisplayedName);
						} else {
							sprintf_s(szTemp,"<c \"#%06X\">%s </c>",realcolor,pPlayer->Data.DisplayedName);
						}
						CXStr NewText(szTemp);
						((CStmlWnd*)pToolTipWnd->Display)->SetSTMLText(NewText,1,0); 
						DWORD typecolor = GetSpawnType((PSPAWNINFO)pPlayer);
						PCHAR sthetype = GetSpawnTypeName(&typecolor);
						sprintf_s(szTemp,"<BR>Level %i %s <c \"#%06X\"><%s></c>",pPlayer->Data.Level,GetClassDesc(pPlayer->Data.mActorClient.Class),typecolor,sthetype);
						if(gGuildOn) {
							char *szGuild = GetGuildByID(pPlayer->Data.GuildID);
							if(PCHAR theguild = szGuild) {
								sprintf_s(szTemp,"<BR><c \"#00FF00\">GUILD:</c>%s<BR>Level %i %s <c \"#%06X\"><%s></c>",theguild,pPlayer->Data.Level,GetClassDesc(pPlayer->Data.mActorClient.Class),typecolor,sthetype);
							}
						}
						((CStmlWnd*)pToolTipWnd->Display)->AppendSTML(szTemp);
						
						LONG maxhp=pSpawn->HPMax;
						DWORD hppct = 0;
						if (maxhp!=0) {
							hppct=pSpawn->HPCurrent*100/maxhp;
						}
						LONG maxmana=pSpawn->ManaMax;
						DWORD manapct = 0;
						if (maxmana!=0) {
							manapct=pSpawn->ManaCurrent*100/maxmana;
						}
						sprintf_s(szTemp,"<BR>ID: %i <c \"#FF69B4\">HP:</c> %d%%<c \"#0080FF\">Mana:</c> %d%%<BR>Loc: %.2f, %.2f, %.2f<br>Distance: %.2f",pPlayer->Data.SpawnID,hppct,manapct,pPlayer->Data.X,pPlayer->Data.Y,pPlayer->Data.Z,GetDistance3D(pSpawn->X,pSpawn->Y,pSpawn->Z,pPlayer->Data.X,pPlayer->Data.Y,pPlayer->Data.Z));
						((CStmlWnd*)pToolTipWnd->Display)->AppendSTML(szTemp);

						((CStmlWnd*)pToolTipWnd->Display)->ForceParseNow();
						if(bRender==1) {
							((CXWnd *)pToolTipWnd)->Show(1,1);
						}
					}
				}
			}
		}
	}
}

PCHAR GetSpawnTypeName(LPDWORD type)
{
	DWORD thetype = *type;
	*type = 0x7F7F7F;
	switch(thetype)
    {
        case MOUNT:
            return "Mount";
        case UNTARGETABLE:
            return "Untargetable";
        case NPC:
			*type = 0xFF69B4;
            return "NPC";
        case PC:
			*type = 0xFF00FF;
            return "PLAYER";
        case CHEST:
            return "CHEST";
        case TRAP:
			*type = 0xFF69B4;
            return "TRAP";
        case PET:
			*type = 0xFFFF00;
            return "PET";
        case ITEM:
            return "Item";
        case CORPSE:
            return "Corpse";
        case OBJECT:
            return "Object";
        case MERCENARY:
            *type = 0xFFFF00;
			return "Mercenary";
		default:
			return "Unknown";
	};
}

PLUGIN_API VOID OnPulse(VOID)
{
	if (gGameState != GAMESTATE_INGAME)
		return;
	if (gGameState == GAMESTATE_INGAME && pCharSpawn) {
		if (!pToolTipWnd) {
			CreateToolTipWindow();
		}
	}
	if(gEnabled==FALSE)
		return;
	if(PSPAWNINFO ps = GetCharInfo()->pSpawn) {
		if (oldX != pMousePos->X || oldY != pMousePos->Y) {
			MouseHover = FALSE;
			oldX = pMousePos->X;
			oldY = pMousePos->Y;
			Time = ps->TimeStamp;
		}
		if (ps->TimeStamp - Time > DelayMs) {
			MouseHover = TRUE;
		}
	}
	if(pToolTipWnd && !((PCSIDLWND)pToolTipWnd)->MouseOver) {
		if(!EqualRect(&pToolTipWnd->Location,&gOldLocation)) {
			CopyRect(&gOldLocation,&pToolTipWnd->Location);
			SaveWindowSettings((PCSIDLWND)pToolTipWnd);
		}
	}
}

VOID LoadWindowSettings(PCSIDLWND pWindow) 
{ 
    CHAR szTemp[MAX_STRING]={0}; 

    strcpy_s(szToolTipINISection,"Default"); 

    pWindow->Location.top		= GetPrivateProfileInt(szToolTipINISection,"WindowTop",       100,INIFileName); 
	pWindow->Location.left		= GetPrivateProfileInt(szToolTipINISection,"WindowLeft",   200,INIFileName); 
	pWindow->Location.right		= GetPrivateProfileInt(szToolTipINISection,"WindowRight",      400,INIFileName); 
	pWindow->Location.bottom	= GetPrivateProfileInt(szToolTipINISection,"WindowBottom",    200,INIFileName); 
	CopyRect(&gOldLocation,&pWindow->Location); 
    pWindow->Locked				= (GetPrivateProfileInt(szToolTipINISection,"Locked",         0,INIFileName) ? true:false); 
    pWindow->Fades				= (GetPrivateProfileInt(szToolTipINISection,"Fades",          1,INIFileName) ? true:false); 
    pWindow->FadeDelay		= GetPrivateProfileInt(szToolTipINISection,"Delay",       2000,INIFileName); 
    pWindow->FadeDuration		= GetPrivateProfileInt(szToolTipINISection,"Duration",     500,INIFileName); 
    pWindow->Alpha				= GetPrivateProfileInt(szToolTipINISection,"Alpha",        200,INIFileName); 
    pWindow->FadeToAlpha		= GetPrivateProfileInt(szToolTipINISection,"FadeToAlpha",  255,INIFileName); 
    pWindow->BGType				= GetPrivateProfileInt(szToolTipINISection,"BGType",         1,INIFileName); 
	ARGBCOLOR col = { 0 };
	col.A			= GetPrivateProfileInt(szToolTipINISection,"BGTint.alpha",   255,INIFileName); 
	col.R			= GetPrivateProfileInt(szToolTipINISection,"BGTint.red",   0,INIFileName); 
    col.G			= GetPrivateProfileInt(szToolTipINISection,"BGTint.green", 0,INIFileName); 
    col.B			= GetPrivateProfileInt(szToolTipINISection,"BGTint.blue",  0,INIFileName); 
	pWindow->BGColor = col.ARGB;
    gFollowMouse				= GetPrivateProfileInt(szToolTipINISection,"FollowMouse",  0,INIFileName); 
    gEnabled					= GetPrivateProfileInt(szToolTipINISection,"Enabled",	   1,INIFileName); 
    gAutoClear					= GetPrivateProfileInt(szToolTipINISection,"AutoClear",	   1,INIFileName); 
    gClearTimer					= GetPrivateProfileInt(szToolTipINISection,"ClearTimer",   1000,INIFileName); 
    gGuildOn					= GetPrivateProfileInt(szToolTipINISection,"GuildOn",	   1,INIFileName); 
}
template <unsigned int _Size>LPSTR SafeItoa(int _Value,char(&_Buffer)[_Size], int _Radix)
{
	errno_t err = _itoa_s(_Value, _Buffer, _Radix);
	if (!err) {
		return _Buffer;
	}
	return "";
}
VOID SaveWindowSettings(PCSIDLWND pWindow) 
{ 
    CHAR szTemp[MAX_STRING]={0}; 
    strcpy_s(szToolTipINISection,"Default"); 

    WritePrivateProfileStringA(szToolTipINISection,"WindowTop",    SafeItoa(pWindow->Location.top,		szTemp,10),INIFileName); 
    WritePrivateProfileStringA(szToolTipINISection,"WindowLeft",   SafeItoa(pWindow->Location.left,		szTemp,10),INIFileName); 
    WritePrivateProfileStringA(szToolTipINISection,"WindowRight",  SafeItoa(pWindow->Location.right,		szTemp,10),INIFileName); 
    WritePrivateProfileStringA(szToolTipINISection,"WindowBottom", SafeItoa(pWindow->Location.bottom,	szTemp,10),INIFileName); 
    WritePrivateProfileStringA(szToolTipINISection,"Locked",       SafeItoa(pWindow->Locked,				szTemp,10),INIFileName); 
    WritePrivateProfileStringA(szToolTipINISection,"Fades",        SafeItoa(pWindow->Fades,				szTemp,10),INIFileName); 
    WritePrivateProfileStringA(szToolTipINISection,"Delay",        SafeItoa(pWindow->FadeDelay,			szTemp,10),INIFileName); 
    WritePrivateProfileStringA(szToolTipINISection,"Duration",     SafeItoa(pWindow->FadeDuration,		szTemp,10),INIFileName); 
    WritePrivateProfileStringA(szToolTipINISection,"Alpha",        SafeItoa(pWindow->Alpha,				szTemp,10),INIFileName); 
    WritePrivateProfileStringA(szToolTipINISection,"FadeToAlpha",  SafeItoa(pWindow->FadeToAlpha,		szTemp,10),INIFileName); 
    WritePrivateProfileStringA(szToolTipINISection,"BGType",       SafeItoa(pWindow->BGType,				szTemp,10),INIFileName); 
	ARGBCOLOR col = { 0 };
	col.ARGB = pWindow->BGColor;
	WritePrivateProfileStringA(szToolTipINISection,"BGTint.alpha",   SafeItoa(col.A,			szTemp,10),INIFileName); 
	WritePrivateProfileStringA(szToolTipINISection,"BGTint.red",   SafeItoa(col.R,			szTemp,10),INIFileName); 
    WritePrivateProfileStringA(szToolTipINISection,"BGTint.green", SafeItoa(col.G,			szTemp,10),INIFileName); 
    WritePrivateProfileStringA(szToolTipINISection,"BGTint.blue",  SafeItoa(col.B,			szTemp,10),INIFileName); 
    WritePrivateProfileStringA(szToolTipINISection,"FollowMouse",  SafeItoa(gFollowMouse,				szTemp,10),INIFileName);  
    WritePrivateProfileStringA(szToolTipINISection,"Enabled",	  SafeItoa(gEnabled,					szTemp,10),INIFileName);  
    WritePrivateProfileStringA(szToolTipINISection,"AutoClear",	  SafeItoa(gAutoClear,					szTemp,10),INIFileName);  
    WritePrivateProfileStringA(szToolTipINISection,"ClearTimer",	  SafeItoa(gClearTimer,					szTemp,10),INIFileName);  
    WritePrivateProfileStringA(szToolTipINISection,"GuildOn",	  SafeItoa(gGuildOn,					szTemp,10),INIFileName);  
}

PLUGIN_API VOID OnCleanUI(VOID)
{
    DebugSpewAlways("MQ2ToolTip::OnCleanUI()");
	DestroyToolTipWindow();
}

// Called once directly after the game ui is reloaded, after issuing /loadskin
PLUGIN_API VOID OnReloadUI(VOID)
{
    DebugSpewAlways("MQ2ToolTip::OnReloadUI()");
	if (gGameState == GAMESTATE_INGAME && pCharSpawn) {
		CreateToolTipWindow();
	}
}


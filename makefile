!include "../global.mak"

ALL : "$(OUTDIR)\MQ2ToolTip.dll"

CLEAN :
	-@erase "$(INTDIR)\MQ2ToolTip.obj"
	-@erase "$(INTDIR)\vc60.idb"
	-@erase "$(OUTDIR)\MQ2ToolTip.dll"
	-@erase "$(OUTDIR)\MQ2ToolTip.exp"
	-@erase "$(OUTDIR)\MQ2ToolTip.lib"
	-@erase "$(OUTDIR)\MQ2ToolTip.pdb"


LINK32=link.exe
LINK32_FLAGS=kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib $(DETLIB) ..\Release\MQ2Main.lib /nologo /dll /incremental:no /pdb:"$(OUTDIR)\MQ2ToolTip.pdb" /debug /machine:I386 /out:"$(OUTDIR)\MQ2ToolTip.dll" /implib:"$(OUTDIR)\MQ2ToolTip.lib" /OPT:NOICF /OPT:NOREF 
LINK32_OBJS= \
	"$(INTDIR)\MQ2ToolTip.obj" \
	"$(OUTDIR)\MQ2Main.lib"

"$(OUTDIR)\MQ2ToolTip.dll" : "$(OUTDIR)" $(DEF_FILE) $(LINK32_OBJS)
    $(LINK32) $(LINK32_FLAGS) $(LINK32_OBJS)


!IF "$(NO_EXTERNAL_DEPS)" != "1"
!IF EXISTS("MQ2ToolTip.dep")
!INCLUDE "MQ2ToolTip.dep"
!ELSE 
!MESSAGE Warning: cannot find "MQ2ToolTip.dep"
!ENDIF 
!ENDIF 


SOURCE=.\MQ2ToolTip.cpp

"$(INTDIR)\MQ2ToolTip.obj" : $(SOURCE) "$(INTDIR)"


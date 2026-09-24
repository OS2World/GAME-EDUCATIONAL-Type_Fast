# makefile.wat - OpenWatcom wmake for TypeFast
# Invoke from src/ directory via compile-wat.cmd

CC   = wcc386
RC   = rc
LINK = wlink

CFLAGS = -bt=os2 -mf -5 -fpi -Oaxt -W3 -ze -d0
LFLAGS = system os2v2 pm op map op quiet

TARGET = ..\bin\typefast.exe
OBJ    = typefast.obj

all: ..\bin $(TARGET)

..\bin: .SYMBOLIC
	-mkdir ..\bin

$(TARGET): $(OBJ) typefast.def typefast.res
	$(LINK) $(LFLAGS) name $(TARGET) file $(OBJ) modfile typefast.def
	$(RC) typefast.res $(TARGET)

$(OBJ): typefast.c typefast.h
	$(CC) $(CFLAGS) typefast.c

typefast.res: typefast.rc typefast.h typefast.ico
	$(RC) -r -I "/@unixroot/usr/include" typefast.rc

clean: .SYMBOLIC
	-rm -f $(OBJ) typefast.res ..\bin\typefast.exe typefast.map

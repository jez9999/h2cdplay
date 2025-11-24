# MS NMake makefile (build with 'nmake' or 'nmake rebuild' to force rebuild each time)
OUTDIR = .

# MSVC build (Win32)
BASSPATH = C:\Development\h2cdplay\bass24\c
BASSLIB = "$(BASSPATH)\bass.lib"
CC = cl
RM = del /Q

# Compiler flags
CFLAGS = /nologo /W3 /O2 /TC /I"$(BASSPATH)" /D_CRT_SECURE_NO_WARNINGS

# Linker flags
LDFLAGS = /nologo

# Pattern rule: compile .c to .exe
# $< = source file, $@ = target executable
.c.exe:
	$(CC) $(CFLAGS) $< /Fe$(OUTDIR)\$@ /link $(BASSLIB) $(LDFLAGS)

TARGET = h2cdplay.exe h2cdplay_cli.exe

all: $(TARGET)

clean:
	$(RM) $(OUTDIR)\$(TARGET) *.obj

rebuild: clean all

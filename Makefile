# MS NMake makefile
OUTDIR = .

# MSVC build (Win32)
BASSPATH = C:\Development\h2cdplay\bass24\c
BASSLIB = "$(BASSPATH)\bass.lib"
CC = cl
RM = del /Q

# Compiler flags
CFLAGS = /nologo /W3 /O2 /TC /I"$(BASSPATH)"

# Link flags
LDFLAGS = /nologo

# Pattern rule: build .exe from .c
{$(OUTDIR)}.c.exe:
	$(CC) $(CFLAGS) $< $(BASSLIB) $(LDFLAGS) /Fe$@

# Explicit pattern rule (more reliable)
.c.exe:
	$(CC) $(CFLAGS) $< $(BASSLIB) $(LDFLAGS) /Fe$(OUTDIR)\$@

.PHONY: all clean

TARGET = h2cdplay.exe h2cdplay_cli.exe

all: $(TARGET)

clean:
	$(RM) $(OUTDIR)\$(TARGET)

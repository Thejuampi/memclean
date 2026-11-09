@echo off
setlocal
cd /d "%~dp0"

set CFLAGS=-std=c23 -Os -s -Wall -Wextra -ffreestanding -fno-builtin -fno-stack-protector -mno-stack-arg-probe -ffunction-sections -fdata-sections -nostdlib -nostartfiles
set LIBS=-lkernel32 -ladvapi32 -lshell32

gcc src\memclean_core.c src\memclean_cli.c src\memclean_engine.c src\memclean_report.c src\memclean_runtime.c -o memclean-cli.exe %CFLAGS% -Wl,-e,entry -Wl,--gc-sections %LIBS%

if errorlevel 1 (
    echo CLI build failed.
    exit /b 1
)

windres src\memclean.rc -O coff -o memclean.res

if errorlevel 1 (
    echo Resource build failed.
    exit /b 1
)

gcc src\memclean_core.c src\memclean_gui.c src\memclean_host.c src\memclean_view.c src\memclean_strings.c src\memclean_engine.c src\memclean_report.c src\memclean_runtime.c memclean.res -o memclean.exe %CFLAGS% -Wl,-e,entry -Wl,--gc-sections -Wl,--subsystem,windows %LIBS% -luser32 -lgdi32 -lgdiplus -lole32

if errorlevel 1 (
    echo GUI build failed.
    exit /b 1
)

del /q memclean.res
echo Built memclean.exe and memclean-cli.exe


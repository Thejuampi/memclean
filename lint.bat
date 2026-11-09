@echo off
setlocal
set "FORMAT=clang-format"
where clang-format >nul 2>nul || set "FORMAT=C:\Program Files\LLVM\bin\clang-format.exe"
set "FAILED="
for %%F in (src\*.c src\*.h) do (
    "%FORMAT%" --dry-run --Werror --style=file "%%F" || set "FAILED=1"
)
if defined FAILED exit /b 1
exit /b 0

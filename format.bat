@echo off
setlocal
set "FORMAT=clang-format"
where clang-format >nul 2>nul || set "FORMAT=C:\Program Files\LLVM\bin\clang-format.exe"
for %%F in (src\*.c src\*.h) do "%FORMAT%" -i --style=file "%%F"

@echo off

IF NOT EXIST bin mkdir bin

cl /Zi src\win32_main.c /Febin\finn /Fdbin\finn /Fobin\finn

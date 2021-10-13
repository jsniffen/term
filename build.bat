@echo off

IF NOT EXIST bin mkdir bin

cl /Zi src\main_windows.c /Febin\finn /Fdbin\finn /Fobin\finn

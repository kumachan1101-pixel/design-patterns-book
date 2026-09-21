@echo off
rem Build and run the sample code of this book. See README.md.
rem   run.bat          show the menu
rem   run.bat verify   check every folder against the book
powershell -NoProfile -ExecutionPolicy Bypass -File "%~dp0run.ps1" %*

@echo off
rem Build and run the sample code of this book. See README.md.
rem   run.bat          show the menu
rem   run.bat all      build and run every sample
powershell -NoProfile -ExecutionPolicy Bypass -File "%~dp0run.ps1" %*

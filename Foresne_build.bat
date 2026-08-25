@echo off
setlocal

:: ============================================================
::  build.bat -- Forensic UEFI Boot Tool
::  Compila los cuatro modulos EFI y organiza los binarios
::  en la estructura correcta para el USB de arranque.
::
::  Requisito: ejecutar edksetup.bat antes de este script.
::
::  Resultado en output_efi\:
::    EFI\Boot\BOOTX64.EFI           <- menu principal
::    EFI\Boot	ools\Files_shows.efi
::    EFI\Boot	ools\Show_info.efi
::    EFI\Boot	ools\Forensic_tool.efi
::    EFI\Boot	ools\whitelist.txt
:: ============================================================

:: --- Configuracion (edita aqui si usas VS2022) ---------------
set ARCH=X64
set TARGET=DEBUG
set TOOLCHAIN=VS2019
set PACKAGE=ForensicPkg\ForensicPkg.dsc
set OUT_DIR=output_efi
set BUILD_PATH=Build\ForensicPkg\%TARGET%_%TOOLCHAIN%\%ARCH%

:: ------------------------------------------------------------
echo.
echo ============================================================
echo   FORENSIC UEFI BOOT TOOL - Script de compilacion
echo ============================================================
echo.

:: Verificar entorno EDK II
if not defined EDK_TOOLS_PATH (
    echo [ERROR] El entorno EDK II no esta inicializado.
    echo         Ejecuta primero: edksetup.bat
    echo.
    pause
    exit /b 1
)

echo [i] Arquitectura : %ARCH%
echo [i] Target       : %TARGET%
echo [i] Toolchain    : %TOOLCHAIN%
echo [i] Paquete      : %PACKAGE%
echo.

:: --- Compilar ------------------------------------------------
echo [*] Compilando ForensicPkg...
echo.

build -p %PACKAGE% -a %ARCH% -t %TOOLCHAIN% -b %TARGET%

if %ERRORLEVEL% neq 0 (
    echo.
    echo [ERROR] Compilacion fallida. Revisa los errores anteriores.
    pause
    exit /b 1
)

echo.
echo [+] Compilacion correcta.
echo.

:: --- Crear estructura de salida ------------------------------
echo [*] Organizando binarios en %OUT_DIR%\...

if exist %OUT_DIR% rmdir /s /q %OUT_DIR%

mkdir %OUT_DIR%\EFI\Boot	ools

:: Menu principal -> BOOTX64.EFI
copy /Y %BUILD_PATH%\Menu_kit_TFM.efi %OUT_DIR%\EFI\Boot\BOOTX64.EFI
if %ERRORLEVEL% neq 0 (
    echo [ERROR] No se encontro Menu_kit_TFM.efi en %BUILD_PATH%
    pause
    exit /b 1
)
echo [+] Menu_kit_TFM.efi   --^>  EFI\Boot\BOOTX64.EFI

:: Modulos tools
copy /Y %BUILD_PATH%\Files_shows.efi   %OUT_DIR%\EFI\Boot	ools\Files_shows.efi
echo [+] Files_shows.efi    --^>  EFI\Boot	ools
copy /Y %BUILD_PATH%\Show_info.efi     %OUT_DIR%\EFI\Boot	ools\Show_info.efi
echo [+] Show_info.efi      --^>  EFI\Boot	ools
copy /Y %BUILD_PATH%\Forensic_Tool.efi %OUT_DIR%\EFI\Boot	ools\Forensic_tool.efi
echo [+] Forensic_Tool.efi  --^>  EFI\Boot	ools
:: Whitelist
if exist ForensicPkg	ools\whitelist.txt (
    copy /Y ForensicPkg	ools\whitelist.txt %OUT_DIR%\EFI\Boot	ools\whitelist.txt
    echo [+] whitelist.txt      --^>  EFI\Boot	ools) else (
    echo [i] whitelist.txt no encontrada, copia manualmente al USB.
)

:: --- Resumen -------------------------------------------------
echo.
echo ============================================================
echo   Binarios listos en: %OUT_DIR%echo ============================================================
echo.
echo   EFI\Boot\BOOTX64.EFI            (menu principal)
echo   EFI\Boot	ools\Files_shows.efi
echo   EFI\Boot	ools\Show_info.efi
echo   EFI\Boot	ools\Forensic_tool.efi
echo   EFI\Boot	ools\whitelist.txt
echo.
echo   Copia el contenido de %OUT_DIR%\ a la raiz
echo   de un USB formateado en FAT32 y arranca desde el.
echo ============================================================
echo.

pause
endlocal

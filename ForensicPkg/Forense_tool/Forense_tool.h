/** @file
  Forensic Extra - Reporting, NVRAM, GPT y TPM2 para el conjunto de
  herramientas UEFI Forensic Boot Tool.

  Modulo standalone: se enlaza como aplicacion UEFI independiente para
  poder ser cargado por el menu central via LoadImage/StartImage.

  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#ifndef __FORENSE_TOOL_H__
#define __FORENSE_TOOL_H__

#include <Uefi.h>

#include <Protocol/BlockIo.h>
#include <Protocol/SimpleFileSystem.h>
#include <Protocol/LoadedImage.h>
#include <Guid/FileInfo.h>
#include <Guid/GlobalVariable.h>
#include <Guid/ImageAuthentication.h>


//
// Colores de consola (mismo esquema que el resto de modulos)
//
#define COLOR_TITLE       EFI_YELLOW
#define COLOR_MENU        EFI_WHITE
#define COLOR_HIGHLIGHT   EFI_GREEN
#define COLOR_ERROR       EFI_RED
#define COLOR_INFO        EFI_CYAN

//
// Tamanos
//
#define MAX_FILE_ENTRIES        32
#define MAX_FILE_PATH_LEN       1024
#define CONSOLE_MSG_BUFFER_SIZE 512
#define MAX_SCAN_DEPTH          6
#define LOG_FILE_NAME           L"\\EFI\\Boot\\forensic_log.txt"
#define WHITELIST_FILE_NAME     L"\\EFI\\Boot\\tools\\whitelist.txt"

//
// Entrada de archivo EFI encontrada durante el escaneo
//
typedef struct {
  EFI_HANDLE  DeviceHandle;
  CHAR16      FilePath[MAX_FILE_PATH_LEN];
  UINT64      FileSize;
  UINT8       Hash[32];
  BOOLEAN     HashValid;
} EFI_FILE_ENTRY;


/*
Lo que sea
*/




//
// Utilidades de consola (identicas al resto de modulos)
//
VOID EFIAPI ClearScreen (VOID);
VOID EFIAPI PrintTitle (IN CONST CHAR16 *Title);
VOID EFIAPI PrintSuccess (IN CONST CHAR16 *Format, ...);
VOID EFIAPI PrintError (IN CONST CHAR16 *Format, ...);
VOID EFIAPI PrintInfo (IN CONST CHAR16 *Format, ...);
VOID EFIAPI Pause (VOID);
INT32 EFIAPI GetMenuChoice (VOID);
VOID EFIAPI ShowMainMenu (VOID);

//
// Escaneo de archivos .EFI (auto-contenido en este modulo)
//
UINTN EFIAPI FindEfiFiles (IN OUT EFI_FILE_ENTRY *Entries, IN UINTN MaxEntries, IN BOOLEAN ComputeHash);

//
// Persistencia / reporting
//
VOID EFIAPI GenerateForensicReport (VOID);
VOID EFIAPI CompareAgainstWhitelist (VOID);

//
// NVRAM / arranque
//
VOID EFIAPI ShowBootVariables (VOID);
VOID EFIAPI ShowSecureBootVariables (VOID);

//
// Estructura de disco
//
VOID EFIAPI ShowGptPartitions (VOID);

//
// TPM2
//
VOID EFIAPI ShowTpm2Pcrs (VOID);

//
// Entrada principal
//
EFI_STATUS EFIAPI UefiMain (IN EFI_HANDLE ImageHandle, IN EFI_SYSTEM_TABLE *SystemTable);

#endif // __FORENSIC_EXTRA_H__

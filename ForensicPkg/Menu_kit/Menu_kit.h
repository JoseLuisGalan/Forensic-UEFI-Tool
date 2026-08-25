/** @file
  Forensic UEFI Boot Tool - Header

  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#ifndef __MENU_KIT_H__
#define __MENU_KIT_H__

#include <Uefi.h>

//
// Protocolos necesarios para los tipos utilizados por el header.
//
#include <Protocol/BlockIo.h>
#include <Protocol/SimpleFileSystem.h>
#include <Guid/FileInfo.h>

//
// Colores de consola
//

#define COLOR_TITLE       EFI_YELLOW
#define COLOR_MENU        EFI_WHITE
#define COLOR_HIGHLIGHT   EFI_GREEN
#define COLOR_ERROR       EFI_RED
#define COLOR_INFO        EFI_CYAN

//
// Tamaños utilizados por el programa.
//

#define MAX_FILE_ENTRIES       32
#define MAX_FILE_PATH_LEN      1024
#define CONSOLE_MSG_BUFFER_SIZE 512
#define MAX_SCAN_DEPTH  5

//
// Entrada de archivo EFI encontrada durante el escaneo.
//
typedef struct {
  EFI_HANDLE  DeviceHandle;
  CHAR16      FilePath[MAX_FILE_PATH_LEN];
  UINT64      FileSize;
} EFI_FILE_ENTRY;

//
// Utilidades de consola
//

VOID
EFIAPI
ClearScreen (
  VOID
  );

VOID
EFIAPI
PrintTitle (
  IN CONST CHAR16  *Title
  );

VOID
EFIAPI
PrintLine (
  IN CONST CHAR16  *Text
  );

VOID
EFIAPI
PrintSuccess (
  IN CONST CHAR16  *Format,
  ...
  );

VOID
EFIAPI
PrintError (
  IN CONST CHAR16  *Format,
  ...
  );

VOID
EFIAPI
PrintInfo (
  IN CONST CHAR16  *Format,
  ...
  );

VOID
EFIAPI
Pause (
  VOID
  );

  INT32
EFIAPI
GetMenuChoice (
  VOID
  );

//
// Búsqueda de archivos .EFI
//



//
// Opciones del menú
//


VOID
EFIAPI
LoadEFIFromDisk (
  IN CONST CHAR16  *FileName
  );



EFI_STATUS
EFIAPI
UefiMain (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  );
  



#endif // __FORENSIC_BOOT_H__
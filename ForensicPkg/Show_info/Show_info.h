#ifndef __SHOW_INFO_H__
#define __SHOW_INFO_H__

#include <Uefi.h>
#include <Library/UefiLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/UefiRuntimeServicesTableLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/PrintLib.h>
#include <Protocol/BlockIo.h>
#include <Protocol/SimpleFileSystem.h>
#include <Guid/GlobalVariable.h>
#include <Protocol/LoadedImage.h>

//
// Colores de consola
//

#define COLOR_TITLE     EFI_YELLOW
#define COLOR_MENU      EFI_WHITE
#define COLOR_HIGHLIGHT EFI_GREEN
#define COLOR_ERROR     EFI_RED
#define COLOR_INFO      EFI_CYAN

//
// Tamaños utilizados por el programa.
//

#define MAX_FILE_ENTRIES       1000
#define MAX_FILE_PATH_LEN      1024
#define CONSOLE_MSG_BUFFER_SIZE 512

//
// Funciones de pantalla
//

VOID
EFIAPI
ClearScreen(
    VOID
);

VOID
EFIAPI
PrintTitle(
    IN CONST CHAR16 *Title
);

VOID
EFIAPI
PrintLine(
    IN CONST CHAR16 *Text
);

VOID
EFIAPI
PrintSuccess(
    IN CONST CHAR16 *Text,
    ...
);

VOID
EFIAPI
PrintError(
    IN CONST CHAR16 *Text,
    ...
);

VOID
EFIAPI
PrintInfo(
    IN CONST CHAR16 *Text,
    ...
);

VOID
EFIAPI
Pause(
    VOID
);

//
// Funcionalidades generales
//

EFI_STATUS
EFIAPI
Create_directory (
  EFI_HANDLE ImageHandle
);

VOID
EFIAPI
CheckSecureBoot (
  VOID
);

VOID
EFIAPI 
ShowFirmwareInfo(
    VOID
);

VOID
EFIAPI
DumpMemory(
    VOID
);

//
// Funcionalidades de Configuracion del Sistema
//

VOID
EFIAPI
ShowUEFIInfo (
  VOID
);

VOID
EFIAPI
ShowBootVariables (
  VOID
);

VOID
EFIAPI
ShowSecureBootConfig (
  VOID
);

VOID
EFIAPI
ListUEFIVariables (
  VOID
);

VOID
EFIAPI
ShowHardwareInfo (
  VOID
);

VOID
EFIAPI 
SystemConfiguration(
    VOID
);

//
// Menu principal
//

VOID
EFIAPI 
ShowMainMenu(
    VOID
);

INT32
EFIAPI
GetMenuChoice(
    VOID
);

EFI_STATUS
EFIAPI
UefiMain(
    IN EFI_HANDLE        ImageHandle,
    IN EFI_SYSTEM_TABLE  *SystemTable
);

#endif
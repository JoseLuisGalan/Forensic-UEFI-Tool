
#include <Uefi.h>

#include <Library/UefiApplicationEntryPoint.h>
#include <Library/UefiLib.h>                    // Print, gST
#include <Library/UefiBootServicesTableLib.h>    // gBS
#include <Library/UefiRuntimeServicesTableLib.h> // gRT (si usas ResetSystem)
#include <Library/MemoryAllocationLib.h>         // FreePool
#include <Library/BaseMemoryLib.h>
#include <Library/BaseLib.h>
#include <Library/PrintLib.h>                    // UnicodeSPrint, UnicodeVSPrint
#include <Library/DevicePathLib.h>                // FileDevicePath

#include <Protocol/LoadedImage.h>                 // EFI_LOADED_IMAGE_PROTOCOL, gEfiLoadedImageProtocolGuid

#include "Menu_kit.h"  

/*

TEXT ZONE / MENU


*/


VOID
EFIAPI
ClearScreen (
  VOID
  )
{
  gST->ConOut->ClearScreen (gST->ConOut);
}

VOID
EFIAPI
PrintTitle (
  IN CONST CHAR16  *Title
  )
{
  gST->ConOut->SetAttribute (gST->ConOut, COLOR_TITLE);
  Print (L"\n====================================================\n");
  Print (L" %-36s \n", Title);
  Print (L"====================================================\n\n");
  gST->ConOut->SetAttribute (gST->ConOut, COLOR_MENU);
}

VOID
EFIAPI
PrintLine (
  IN CONST CHAR16  *Text
  )
{
  Print (L"%s\n", Text);
}

VOID
EFIAPI
PrintSuccess (
  IN CONST CHAR16  *Format,
  ...
  )
{
  VA_LIST  Marker;
  CHAR16   Buffer[CONSOLE_MSG_BUFFER_SIZE];

  VA_START (Marker, Format);
  UnicodeVSPrint (Buffer, sizeof (Buffer), Format, Marker);
  VA_END (Marker);

  gST->ConOut->SetAttribute (gST->ConOut, COLOR_HIGHLIGHT);
  Print (L"[+] %s\n", Buffer);
  gST->ConOut->SetAttribute (gST->ConOut, COLOR_MENU);
}

VOID
EFIAPI
PrintError (
  IN CONST CHAR16  *Format,
  ...
  )
{
  VA_LIST  Marker;
  CHAR16   Buffer[CONSOLE_MSG_BUFFER_SIZE];

  VA_START (Marker, Format);
  UnicodeVSPrint (Buffer, sizeof (Buffer), Format, Marker);
  VA_END (Marker);

  gST->ConOut->SetAttribute (gST->ConOut, COLOR_ERROR);
  Print (L"[x] %s\n", Buffer);
  gST->ConOut->SetAttribute (gST->ConOut, COLOR_MENU);
}

VOID
EFIAPI
PrintInfo (
  IN CONST CHAR16  *Format,
  ...
  )
{
  VA_LIST  Marker;
  CHAR16   Buffer[CONSOLE_MSG_BUFFER_SIZE];

  VA_START (Marker, Format);
  UnicodeVSPrint (Buffer, sizeof (Buffer), Format, Marker);
  VA_END (Marker);

  gST->ConOut->SetAttribute (gST->ConOut, COLOR_INFO);
  Print (L"[i] %s\n", Buffer);
  gST->ConOut->SetAttribute (gST->ConOut, COLOR_MENU);
}

/**
  Espera a que el usuario pulse una tecla y limpia el buffer de teclado
  para que la siguiente lectura (por ejemplo GetMenuChoice) no reciba
  la misma pulsacion.
**/
VOID
EFIAPI
Pause (
  VOID
  )
{
  EFI_STATUS     Status;
  EFI_INPUT_KEY  Key;
  UINTN          Index;

  Print (L"\n[Presiona cualquier tecla para continuar...]\n");

  gBS->WaitForEvent (1, &gST->ConIn->WaitForKey, &Index);

  //
  // Vaciar el buffer: puede haber mas de un evento encolado.
  //
  do {
    Status = gST->ConIn->ReadKeyStroke (gST->ConIn, &Key);
  } while (!EFI_ERROR (Status));
 
}



VOID
EFIAPI
ShowMainMenu (
  VOID
  )
{
  ClearScreen ();
  gST->ConOut->SetAttribute (gST->ConOut, COLOR_TITLE);
  Print (L"\n");
  Print (L"====================================================\n");
  Print (L"          FORENSIC UEFI BOOT TOOL v1.0\n");
  Print (L"====================================================\n\n");

  gST->ConOut->SetAttribute (gST->ConOut, COLOR_MENU);
  Print (L"----------------------------------------------------\n");
  Print (L"  MENU PRINCIPAL\n");
  Print (L"----------------------------------------------------\n\n");

  Print (L"  [1] Herramienta de listado y cargas de efi\n");
  Print (L"  [2] Variables e informacion del sistema\n");
  Print (L"  [3] Herramientas forenses\n");
  Print (L"  [0] Reiniciar\n\n");

  Print (L"----------------------------------------------------\n");
  Print (L"  Selecciona una opcion (0-2): ");
}

INT32
EFIAPI
GetMenuChoice (
  VOID
  )
{
  EFI_INPUT_KEY  Key;
  UINTN          Index;

  gBS->WaitForEvent (1, &gST->ConIn->WaitForKey, &Index);
  gST->ConIn->ReadKeyStroke (gST->ConIn, &Key);

  if ((Key.UnicodeChar >= L'0') && (Key.UnicodeChar <= L'9')) {
    return (INT32)(Key.UnicodeChar - L'0');
  }

  return -1;
}
/*

TEXT ZONE / MENU

*/

VOID
EFIAPI
LoadEFIFromDisk (
  IN CONST CHAR16  *FileName
  )
{
  EFI_STATUS                 Status;
  EFI_LOADED_IMAGE_PROTOCOL  *LoadedImage;
  CHAR16                     FilePath[MAX_FILE_PATH_LEN];
  EFI_DEVICE_PATH_PROTOCOL   *DevicePath;
  EFI_HANDLE                 LoadedHandle;

  ClearScreen ();
  PrintTitle (L"CARGAR HERRAMIENTA");

  if ((FileName == NULL) || (FileName[0] == L'\0')) {
    PrintError (L"Nombre de archivo no valido");
    Pause ();
    return;
  }

  //
  // Obtener el device handle del propio loader (mismo disco)
  //
  Status = gBS->HandleProtocol (
                  gImageHandle,
                  &gEfiLoadedImageProtocolGuid,
                  (VOID **)&LoadedImage
                  );

  if (EFI_ERROR (Status)) {
    PrintError (L"No se pudo obtener LoadedImageProtocol: %r", Status);
    Pause ();
    return;
  }

  //
  // Construir ruta \tools\<FileName>.efi
  //
  UnicodeSPrint (FilePath, sizeof (FilePath), L"EFI\\Boot\\tools\\%s.efi", FileName);

  PrintInfo (L"Cargando: %s", FilePath);

  DevicePath = FileDevicePath (LoadedImage->DeviceHandle, FilePath);
  if (DevicePath == NULL) {
    PrintError (L"No se pudo construir el device path");
    Pause ();
    return;
  }

  LoadedHandle = NULL;
  Status = gBS->LoadImage (
                  FALSE,
                  gImageHandle,
                  DevicePath,
                  NULL,
                  0,
                  &LoadedHandle
                  );

  FreePool (DevicePath);

  if (EFI_ERROR (Status)) {
    PrintError (L"LoadImage fallo: %r", Status);
    Pause ();
    return;
  }

  PrintSuccess (L"Imagen cargada, iniciando...");
  Pause ();

  Status = gBS->StartImage (LoadedHandle, NULL, NULL);

  if (EFI_ERROR (Status)) {
    PrintError (L"StartImage fallo: %r", Status);
    Pause ();
  }

  Print (L"\n[+] StartImage retorno: %r\n", Status);
  Pause ();
}

EFI_STATUS
EFIAPI
UefiMain(
    IN EFI_HANDLE ImageHandle,
    IN EFI_SYSTEM_TABLE *SystemTable
){
    INT32 Choice;
    BOOLEAN ExitApp;
    gImageHandle = ImageHandle;
    gST->ConOut->EnableCursor(gST->ConOut, FALSE);
    ExitApp = FALSE;

    while (!ExitApp) {
    ShowMainMenu ();
    Choice = GetMenuChoice ();

    switch (Choice) {
      case 1:
        LoadEFIFromDisk (L"Files_shows");
        break;
      case 2:
        LoadEFIFromDisk (L"Show_info");
        break;
      case 3:
        LoadEFIFromDisk (L"Forensic_tool");
        break;
      case 0:
        //Print (L"\nReiniciando sistema...\n");
        //gRT->ResetSystem (EfiResetCold, EFI_SUCCESS, 0, NULL);
        ExitApp = TRUE;
        ClearScreen();
        break;
      default:
        ClearScreen ();
        PrintError (L"Opcion no valida. Intenta de nuevo.");
        Pause ();
        break;
    }
  }

  return EFI_SUCCESS;
}
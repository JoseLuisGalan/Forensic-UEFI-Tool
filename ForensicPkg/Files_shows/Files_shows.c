#include <Uefi.h>
#include "Files_shows.h"
#include <Library/UefiApplicationEntryPoint.h>
#include <Library/UefiLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/UefiRuntimeServicesTableLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/BaseLib.h>
#include <Library/PrintLib.h>
#include <Library/DevicePathLib.h>
#include <Guid/FileInfo.h>
#include <Protocol/BlockIo.h>
#include <Protocol/SimpleFileSystem.h>
#include <Library/BaseCryptLib.h>


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

/*

TEXT ZONE / MENU

*/



STATIC
VOID
ScanDirectoryRecursive (
  IN     EFI_FILE_PROTOCOL  *Dir,
  IN     EFI_HANDLE         DeviceHandle,
  IN     CONST CHAR16       *CurrentPath,
  IN     UINTN              Depth,
  IN OUT EFI_FILE_ENTRY      *Entries,
  IN OUT UINTN              *Found,
  IN     UINTN              MaxEntries
  )
{
  EFI_STATUS  Status;

  if (Depth > MAX_SCAN_DEPTH) {
    return;
  }

  for ( ; *Found < MaxEntries; ) {
    UINTN          BufferSize;
    EFI_FILE_INFO  *FileInfo;
    CHAR16         NewPath[MAX_FILE_PATH_LEN];

    BufferSize = SIZE_OF_EFI_FILE_INFO + MAX_FILE_PATH_LEN * sizeof (CHAR16);
    FileInfo   = AllocatePool (BufferSize);
    if (FileInfo == NULL) {
      break;
    }

    Status = Dir->Read (Dir, &BufferSize, FileInfo);
    if (EFI_ERROR (Status) || BufferSize == 0) {
      FreePool (FileInfo);
      break;
    }

    // Saltar entradas "." y ".."
    if (StrCmp (FileInfo->FileName, L".") == 0 ||
        StrCmp (FileInfo->FileName, L"..") == 0)
    {
      FreePool (FileInfo);
      continue;
    }

    // Construir ruta acumulada
    if (StrCmp (CurrentPath, L"\\") == 0) {
      UnicodeSPrint (NewPath, sizeof (NewPath), L"\\%s", FileInfo->FileName);
    } else {
      UnicodeSPrint (NewPath, sizeof (NewPath), L"%s\\%s", CurrentPath, FileInfo->FileName);
    }

    if (FileInfo->Attribute & EFI_FILE_DIRECTORY) {
      if (Depth < MAX_SCAN_DEPTH) {
        EFI_FILE_PROTOCOL  *SubDir = NULL;

        // Open es relativo al directorio actual, basta el nombre
        Status = Dir->Open (
                        Dir,
                        &SubDir,
                        FileInfo->FileName,
                        EFI_FILE_MODE_READ,
                        0
                        );

        if (!EFI_ERROR (Status)) {
          ScanDirectoryRecursive (
            SubDir,
            DeviceHandle,
            NewPath,
            Depth + 1,
            Entries,
            Found,
            MaxEntries
            );
          SubDir->Close (SubDir);
        }
      }
    } else {
      if (StrStr (FileInfo->FileName, L".efi") != NULL) {
        Entries[*Found].DeviceHandle = DeviceHandle;
        Entries[*Found].FileSize     = FileInfo->FileSize;
        StrCpyS (
          Entries[*Found].FilePath,
          ARRAY_SIZE (Entries[*Found].FilePath),
          NewPath
          );
        (*Found)++;
      }
    }

    FreePool (FileInfo);
  }
}



UINTN
EFIAPI
FindEfiFiles (
  IN OUT EFI_FILE_ENTRY  *Entries,
  IN     UINTN           MaxEntries
  )
{
  EFI_STATUS  Status;
  UINTN       HandleCount;
  EFI_HANDLE  *HandleBuffer;
  UINTN       HandleIndex;
  UINTN       Found;

  HandleCount   = 0;
  HandleBuffer  = NULL;
  Found         = 0;

  Status = gBS->LocateHandleBuffer (
                  ByProtocol,
                  &gEfiSimpleFileSystemProtocolGuid,
                  NULL,
                  &HandleCount,
                  &HandleBuffer
                  );

  if (EFI_ERROR (Status)) {
    return 0;
  }

  for (HandleIndex = 0; (HandleIndex < HandleCount) && (Found < MaxEntries); HandleIndex++) {
    EFI_SIMPLE_FILE_SYSTEM_PROTOCOL  *FileSystem;
    EFI_FILE_PROTOCOL                *Root;


    FileSystem  = NULL;
    Root        = NULL;


    Status = gBS->HandleProtocol (
                    HandleBuffer[HandleIndex],
                    &gEfiSimpleFileSystemProtocolGuid,
                    (VOID **)&FileSystem
                    );

    if (EFI_ERROR (Status)) {
      continue;
    }

    Status = FileSystem->OpenVolume (FileSystem, &Root);
    if (EFI_ERROR (Status)) {
      continue;
    }

    ScanDirectoryRecursive (
    Root,
    HandleBuffer[HandleIndex],
    L"\\",
    0,
    Entries,
    &Found,
    MaxEntries
    );
     Root->Close (Root);
  }

  FreePool (HandleBuffer);

  return Found;
}


VOID
EFIAPI
LoadEFIFromDisk (
  VOID
  )
{
  EFI_FILE_ENTRY             *Entries;
  EFI_FILE_ENTRY             *FilteredEntries;
  UINTN                      Count;
  UINTN                      FilteredCount;
  UINTN                      Index;
  UINTN                      PageSize;
  UINTN                      TotalPages;
  UINTN                      CurrentPage;
  UINTN                      PageStart;
  UINTN                      PageEnd;
  UINTN                      ItemsInPage;
  UINTN                      SelectedIndex;
  BOOLEAN                    Selecting;
  BOOLEAN                    Cancelled;
  EFI_STATUS                 Status;
  EFI_DEVICE_PATH_PROTOCOL   *DevicePath;
  EFI_HANDLE                 LoadedHandle;
  EFI_INPUT_KEY              Key;
  UINTN                      EventIndex;

  ClearScreen ();
  PrintTitle (L"CARGAR EFI DESDE OTRO DISCO");

  PrintInfo (L"Escaneando volumenes en busca de archivos .efi...");
  Print (L"\n");

  Entries = AllocateZeroPool (
              sizeof (EFI_FILE_ENTRY) * MAX_FILE_ENTRIES
              );

  if (Entries == NULL) {
    PrintError (L"No se pudo reservar memoria");
    Pause ();
    return;
  }

  Count = FindEfiFiles (Entries, MAX_FILE_ENTRIES);

  if (Count == 0) {
    PrintError (L"No se encontraron archivos .efi");
    Pause ();
    FreePool (Entries);
    return;
  }

  // FILTRAR POR EFI/Boot
  FilteredEntries = AllocateZeroPool (
                      sizeof (EFI_FILE_ENTRY) * MAX_FILE_ENTRIES
                      );
  if (FilteredEntries == NULL) {
    PrintError (L"No se pudo reservar memoria");
    Pause ();
    FreePool (Entries);
    return;
  }

  FilteredCount = 0;
  for (Index = 0; Index < Count; Index++) {
    // Buscar "EFI\Boot\" en la ruta
    if (StrStr (Entries[Index].FilePath, L"EFI\\") != NULL ||
        StrStr (Entries[Index].FilePath, L"EFI/") != NULL) {
      
      CopyMem (
        &FilteredEntries[FilteredCount],
        &Entries[Index],
        sizeof (EFI_FILE_ENTRY)
        );
      FilteredCount++;
    }
  }

  // Liberar memoria de búsqueda completa
  FreePool (Entries);

  if (FilteredCount == 0) {
    PrintError (L"No se encontraron archivos .efi en EFI/");
    Pause ();
    FreePool (FilteredEntries);
    return;
  }

  PageSize    = 9;
  TotalPages  = (FilteredCount + PageSize - 1) / PageSize;
  CurrentPage = 0;
  Selecting   = TRUE;
  Cancelled   = FALSE;
  SelectedIndex = 0;

  while (Selecting) {
    ClearScreen ();
    PrintTitle (L"CARGAR EFI DESDE OTRO DISCO");

    PageStart   = CurrentPage * PageSize;
    PageEnd     = PageStart + PageSize;
    if (PageEnd > FilteredCount) {
      PageEnd = FilteredCount;
    }
    ItemsInPage = PageEnd - PageStart;

    for (Index = 0; Index < ItemsInPage; Index++) {
      UINTN  GlobalIndex = PageStart + Index;

      Print (
        L"  [%d] %s (Handle %p)\n",
        Index + 1,
        FilteredEntries[GlobalIndex].FilePath,
        FilteredEntries[GlobalIndex].DeviceHandle
        );
    }

    Print (L"\n  Pagina %d de %d  (Total: %d archivos)\n", CurrentPage + 1, TotalPages, FilteredCount);

    if (TotalPages > 1) {
      Print (L"  [<-] Pagina anterior   [->] Pagina siguiente\n");
    }

    Print (L"  [0] Cancelar\n");
    Print (L"\n  Selecciona un archivo a cargar: ");

    gBS->WaitForEvent (1, &gST->ConIn->WaitForKey, &EventIndex);
    gST->ConIn->ReadKeyStroke (gST->ConIn, &Key);

    if ((Key.UnicodeChar >= L'1') && (Key.UnicodeChar <= L'9')) {
      UINTN  Choice = (UINTN)(Key.UnicodeChar - L'0');

      if (Choice <= ItemsInPage) {
        SelectedIndex = PageStart + (Choice - 1);
        Selecting     = FALSE;
      }
    } else if (Key.UnicodeChar == L'0') {
      Cancelled = TRUE;
      Selecting = FALSE;
    } else if ((Key.ScanCode == SCAN_RIGHT) && (TotalPages > 1)) {
      CurrentPage = (CurrentPage + 1) % TotalPages;
    } else if ((Key.ScanCode == SCAN_LEFT) && (TotalPages > 1)) {
      CurrentPage = (CurrentPage == 0) ? (TotalPages - 1) : (CurrentPage - 1);
    }
  }

  if (Cancelled) {
    PrintInfo (L"Cancelado");
    Pause ();
    FreePool (FilteredEntries);
    return;
  }

  DevicePath = FileDevicePath (FilteredEntries[SelectedIndex].DeviceHandle, FilteredEntries[SelectedIndex].FilePath);
  if (DevicePath == NULL) {
    PrintError (L"No se pudo construir el device path");
    Pause ();
    FreePool (FilteredEntries);
    return;
  }

  PrintInfo (L"Cargando imagen...");

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
    FreePool (FilteredEntries);
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

  FreePool (FilteredEntries);
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

  Print (L"  [1] Dispositivos de almacenamiento\n");
  Print (L"  [2] Listado de archivos .EFI\n");
  Print (L"  [3] Cargar .EFI de otros discos\n");
  Print (L"  [0] Reiniciar\n\n");

  Print (L"----------------------------------------------------\n");
  Print (L"  Selecciona una opcion (0-9): ");
}

VOID
EFIAPI
ListStorageDevices (
  VOID
  )
{
  EFI_STATUS  Status;
  UINTN       HandleCount;
  EFI_HANDLE  *HandleBuffer;
  UINTN       Index;
  UINTN       DeviceNum;
  
  ClearScreen ();
  PrintTitle (L"DISPOSITIVOS DE ALMACENAMIENTO");

  HandleCount   = 0;
  HandleBuffer  = NULL;
  DeviceNum     = 1;

  Status = gBS->LocateHandleBuffer (
                  ByProtocol,
                  &gEfiBlockIoProtocolGuid,
                  NULL,
                  &HandleCount,
                  &HandleBuffer
                  );

  if (EFI_ERROR (Status)) {
    PrintError (L"No se encontraron dispositivos de almacenamiento");
    Pause ();
    return;
  }

  
  PrintInfo (L"Se encontraron dispositivos:");
  Print (L"\n");

   for (Index = 0; Index < HandleCount; Index++) {
      EFI_BLOCK_IO_PROTOCOL  *BlockIo;

      BlockIo = NULL;
      Status  = gBS->HandleProtocol (
                     HandleBuffer[Index],
                     &gEfiBlockIoProtocolGuid,
                     (VOID **)&BlockIo
                     );
    if (EFI_ERROR (Status)) {
      continue;
    }

    
    if (!BlockIo->Media->LogicalPartition) {
      UINT64  TotalMb;

     TotalMb = ((BlockIo->Media->LastBlock + 1) * BlockIo->Media->BlockSize) / (1024 * 1024);

      Print (L"  [%d] Dispositivo: Handle %p\n", DeviceNum, HandleBuffer[Index]);
      Print (L"      Tamano: %Lu bloques (%Lu MB)\n", BlockIo->Media->LastBlock + 1, TotalMb);
      Print (L"      Tamano bloque: %d bytes\n", BlockIo->Media->BlockSize);
      Print (
        L"      Estado: %s\n\n",
        BlockIo->Media->MediaPresent ? L"Presente" : L"No presente"
        );
      DeviceNum++;
    }
  }

  FreePool (HandleBuffer);
  Pause ();
}

VOID
EFIAPI
ListEFIFiles (
  VOID
  )
{
  EFI_STATUS                       Status;
  EFI_FILE_ENTRY                   *Entries;
  UINTN                            Count;
  UINTN                            Index;
  UINT8                            Hash[SHA256_DIGEST_SIZE];
  EFI_FILE_PROTOCOL                *File;
  VOID                             *Buffer;
  UINTN                            BufferSize;
  BOOLEAN                          OK;
  EFI_SIMPLE_FILE_SYSTEM_PROTOCOL  *FileSystem;
  EFI_FILE_PROTOCOL                *Root;
  UINTN                            PageSize;
  UINTN                            TotalPages;
  UINTN                            CurrentPage;
  UINTN                            PageStart;
  UINTN                            PageEnd;
  UINTN                            ItemsInPage;
  BOOLEAN                          Navigating;
  EFI_INPUT_KEY                    Key;
  UINTN                            EventIndex;
  UINTN                            i;

  ClearScreen ();
  PrintTitle (L"BUSQUEDA DE ARCHIVOS .EFI");

  PrintInfo (L"Escaneando volumenes...");
  Print (L"\n");

  Entries = AllocateZeroPool (
              sizeof (EFI_FILE_ENTRY) * MAX_FILE_ENTRIES
              );

  if (Entries == NULL) {
    PrintError (L"No se pudo reservar memoria para las entradas EFI");
    Pause ();
    return;
  }

  Count = FindEfiFiles (Entries, MAX_FILE_ENTRIES);

  if (Count == 0) {
    PrintError (L"No se encontraron archivos .efi");
    Pause ();
    FreePool (Entries);
    return;
  }

  PageSize    = 3;
  TotalPages  = (Count + PageSize - 1) / PageSize;
  CurrentPage = 0;
  Navigating  = TRUE;

  while (Navigating) {
    ClearScreen ();
    PrintTitle (L"BUSQUEDA DE ARCHIVOS .EFI");

    PageStart   = CurrentPage * PageSize;
    PageEnd     = PageStart + PageSize;
    if (PageEnd > Count) {
      PageEnd = Count;
    }
    ItemsInPage = PageEnd - PageStart;

    for (Index = PageStart; Index < PageEnd; Index++) {

      //
      // Obtener el filesystem del dispositivo de esta entrada
      //
      FileSystem = NULL;
      Status = gBS->HandleProtocol (
                      Entries[Index].DeviceHandle,
                      &gEfiSimpleFileSystemProtocolGuid,
                      (VOID **)&FileSystem
                      );
      if (EFI_ERROR (Status)) {
        Print (L"  [%d] Error obteniendo filesystem\n", (Index - PageStart + 1));
        continue;
      }

      //
      // Abrir el volumen raiz
      //
      Root = NULL;
      Status = FileSystem->OpenVolume (FileSystem, &Root);
      if (EFI_ERROR (Status)) {
        Print (L"  [%d] Error abriendo volumen\n", (Index - PageStart + 1));
        continue;
      }

      //
      // Reservar buffer para el archivo
      //
      BufferSize = (UINTN)Entries[Index].FileSize;
      Buffer     = AllocatePool (BufferSize);
      if (Buffer == NULL) {
        Print (L"  [%d] Error en la reserva\n", (Index - PageStart + 1));
        Root->Close (Root);
        continue;
      }

      //
      // Abrir el archivo a partir del volumen raiz
      //
      File = NULL;
      Status = Root->Open (
                     Root,
                     &File,
                     Entries[Index].FilePath,
                     EFI_FILE_MODE_READ,
                     0
                     );

      Root->Close (Root);

      if (EFI_ERROR (Status)) {
        Print (L"  [%d] Error en la apertura\n", (Index - PageStart + 1));
        FreePool (Buffer);
        continue;
      }

      //
      // Leer el contenido del archivo
      //
      Status = File->Read (File, &BufferSize, Buffer);
      if (EFI_ERROR (Status)) {
        Print (L"  [%d] Error leyendo el fichero: %r\n", (Index - PageStart + 1), Status);
        FreePool (Buffer);
        File->Close (File);
        continue;
      }

      //
      // Calcular hash
      //
      OK = Sha256HashAll (Buffer, BufferSize, Hash);

      if (OK) {
        Print (L"  [%d] %s\n", (Index - PageStart + 1), Entries[Index].FilePath);
        Print (L"      Handle: %p\n", Entries[Index].DeviceHandle);
        Print (L"      Tamano: %Lu bytes\n", Entries[Index].FileSize);
        Print (L"      Hash: ");
        for (i = 0; i < SHA256_DIGEST_SIZE; i++) {
          Print (L"%02x", Hash[i]);
        }
        Print (L"\n\n");
      } else {
        Print (L"  [%d] %s (Error calculando hash)\n", (Index - PageStart + 1), Entries[Index].FilePath);
        Print (L"      Handle: %p\n\n", Entries[Index].DeviceHandle);
      }

      FreePool (Buffer);
      File->Close (File);
    }

    Print (L"\n  Pagina %d de %d  (Total: %d archivos)\n", CurrentPage + 1, TotalPages, Count);

    if (TotalPages > 1) {
      Print (L"  [<-] Pagina anterior   [->] Pagina siguiente\n");
    }

    Print (L"  [0] Salir\n");
    Print (L"\n  Selecciona una opcion: ");

    gBS->WaitForEvent (1, &gST->ConIn->WaitForKey, &EventIndex);
    gST->ConIn->ReadKeyStroke (gST->ConIn, &Key);

    if (Key.UnicodeChar == L'0') {
      Navigating = FALSE;
    } else if ((Key.ScanCode == SCAN_RIGHT) && (TotalPages > 1)) {
      CurrentPage = (CurrentPage + 1) % TotalPages;
    } else if ((Key.ScanCode == SCAN_LEFT) && (TotalPages > 1)) {
      CurrentPage = (CurrentPage == 0) ? (TotalPages - 1) : (CurrentPage - 1);
    }
  }

  FreePool (Entries);
}
EFI_STATUS
EFIAPI
UefiMain(
    IN EFI_HANDLE ImageHandle,
    IN EFI_SYSTEM_TABLE *SystemTable
)
{
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
        ListStorageDevices ();
        break;
      case 2:
        ListEFIFiles ();
        break;
      case 3:
        LoadEFIFromDisk ();
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
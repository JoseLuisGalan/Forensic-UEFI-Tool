#include "Show_info.h"

/*

Pantalla

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

  do {
    Status = gST->ConIn->ReadKeyStroke (gST->ConIn, &Key);
  } while (!EFI_ERROR (Status));
}

/*

Pantalla

*/

/*

Funcionalidades

*/

EFI_STATUS
EFIAPI
Create_directory (
  EFI_HANDLE ImageHandle
)
{
    EFI_STATUS Status;
    EFI_LOADED_IMAGE_PROTOCOL *LoadedImage;
    EFI_SIMPLE_FILE_SYSTEM_PROTOCOL *FileSystem;
    EFI_FILE_PROTOCOL *Root;
    EFI_FILE_PROTOCOL *Directory;

    Status = gBS->OpenProtocol(
        ImageHandle,
        &gEfiLoadedImageProtocolGuid,
        (VOID **)&LoadedImage,
        ImageHandle,
        NULL,
        EFI_OPEN_PROTOCOL_GET_PROTOCOL
    );

    if (EFI_ERROR(Status))
        return Status;

    Status = gBS->OpenProtocol(
        LoadedImage->DeviceHandle,
        &gEfiSimpleFileSystemProtocolGuid,
        (VOID **)&FileSystem,
        ImageHandle,
        NULL,
        EFI_OPEN_PROTOCOL_GET_PROTOCOL
    );

    if (EFI_ERROR(Status))
        return Status;

    Status = FileSystem->OpenVolume(FileSystem, &Root);

    if (EFI_ERROR(Status))
        return Status;

    Status = Root->Open(
        Root,
        &Directory,
        L"\\EFI\\Boot\\TMP",
        EFI_FILE_MODE_READ |
        EFI_FILE_MODE_WRITE |
        EFI_FILE_MODE_CREATE,
        EFI_FILE_DIRECTORY
    );

    if (!EFI_ERROR(Status)) {
         Print(L"STATUS: %r\n", Status);
  Pause();
        Directory->Close(Directory);
    }

    Root->Close(Root);

    return Status;
}

VOID
EFIAPI
CheckSecureBoot (
  VOID
  )
{
  EFI_STATUS  Status;
  UINT8       SecureBootEnabled;
  UINTN       DataSize;

  ClearScreen ();
  PrintTitle (L"ESTADO DEL SECURE BOOT");

  SecureBootEnabled  = 0;
  DataSize            = sizeof (SecureBootEnabled);

  Status = gRT->GetVariable (
                  L"SecureBoot",
                  &gEfiGlobalVariableGuid,
                  NULL,
                  &DataSize,
                  &SecureBootEnabled
                  );

  Print (L"\n");
  if (!EFI_ERROR (Status)) {
    gST->ConOut->SetAttribute (gST->ConOut, COLOR_HIGHLIGHT);
    if (SecureBootEnabled) {
      Print (L"[*] SECURE BOOT: ACTIVADO\n");
    } else {
      Print (L"[ ] SECURE BOOT: DESACTIVADO\n");
    }
  } else {
    PrintError (L"No se pudo determinar el estado de Secure Boot");
  }

  gST->ConOut->SetAttribute (gST->ConOut, COLOR_MENU);
  Print (L"\n");
  PrintInfo (L"IMPLICACIONES:");
  Print (L"  - Activado: Solo firmware firmado puede ejecutarse\n");
  Print (L"  - Desactivado: Se pueden cargar bootloaders sin firma\n");
  Print (L"\n");
  PrintInfo (L"Para trabajo forense, puede requerir desactivacion");

  Pause ();
}

VOID
EFIAPI
ShowFirmwareInfo (
  VOID
  )
{
  ClearScreen ();
  PrintTitle (L"INFORMACION DEL FIRMWARE");

  Print (L"\nVendedor: %s\n", gST->FirmwareVendor);
  Print (L"Version: %d\n", gST->FirmwareRevision);
  Print (L"\n");
  PrintInfo (L"DATOS DE TABLA DE SISTEMA:");
  Print (L"  - ConIn: %p\n", gST->ConIn);
  Print (L"  - ConOut: %p\n", gST->ConOut);
  Print (L"  - RuntimeServices: %p\n", gST->RuntimeServices);
  Print (L"  - BootServices: %p\n", gST->BootServices);
  Print (L"\n");
  PrintInfo (L"TABLA DE CONFIGURACION:");
  Print (L"  - Numero de entradas: %Lu\n", (UINT64)gST->NumberOfTableEntries);

  Pause ();
}

VOID
EFIAPI
DumpMemory (
  VOID
  )
{
  EFI_STATUS             Status;
  UINTN                  MemoryMapSize;
  EFI_MEMORY_DESCRIPTOR  *MemoryMap;
  UINTN                  MapKey;
  UINTN                  DescriptorSize;
  UINT32                 DescriptorVersion;
  UINTN                  Index;
  UINTN                  RegionCount;
  UINTN                  PageSize;
  UINTN                  TotalPages;
  UINTN                  CurrentPage;
  UINTN                  PageStart;
  UINTN                  PageEnd;
  UINTN                  ItemsInPage;
  BOOLEAN                Navigating;
  EFI_INPUT_KEY          Key;
  UINTN                  EventIndex;
  EFI_MEMORY_DESCRIPTOR  *Desc;
  CONST CHAR16           *TypeStr;
  UINTN                  DisplayIndex;

  ClearScreen ();
  PrintTitle (L"DUMP DE MEMORIA");

  MemoryMapSize = 0;
  MemoryMap     = NULL;
  RegionCount   = 0;

  Status = gBS->GetMemoryMap (
                  &MemoryMapSize,
                  MemoryMap,
                  &MapKey,
                  &DescriptorSize,
                  &DescriptorVersion
                  );

  if (Status != EFI_BUFFER_TOO_SMALL) {
    PrintError (L"No se pudo obtener el tamano del mapa de memoria");
    Pause ();
    return;
  }

  MemoryMapSize += 2 * DescriptorSize;
  MemoryMap      = AllocatePool (MemoryMapSize);

  if (MemoryMap == NULL) {
    PrintError (L"No hay memoria suficiente para el mapa");
    Pause ();
    return;
  }

  Status = gBS->GetMemoryMap (
                  &MemoryMapSize,
                  MemoryMap,
                  &MapKey,
                  &DescriptorSize,
                  &DescriptorVersion
                  );

  if (EFI_ERROR (Status)) {
    PrintError (L"Error obteniendo mapa de memoria: %r", Status);
    Pause ();
    FreePool (MemoryMap);
    return;
  }

  PrintSuccess (L"Mapa de memoria obtenido");
  Print (L"\n");

  for (Index = 0; Index < MemoryMapSize / DescriptorSize; Index++) {
    Desc = (EFI_MEMORY_DESCRIPTOR *)((UINT8 *)MemoryMap + Index * DescriptorSize);

    if ((Desc->Type == EfiConventionalMemory) ||
        (Desc->Type == EfiRuntimeServicesData) ||
        (Desc->Type == EfiBootServicesData))
    {
      RegionCount++;
    }
  }

  if (RegionCount == 0) {
    PrintError (L"No se encontraron regiones de memoria");
    Pause ();
    FreePool (MemoryMap);
    return;
  }

  PageSize    = 3;
  TotalPages  = (RegionCount + PageSize - 1) / PageSize;
  CurrentPage = 0;
  Navigating  = TRUE;

  while (Navigating) {
    ClearScreen ();
    PrintTitle (L"DUMP DE MEMORIA");

    PageStart   = CurrentPage * PageSize;
    PageEnd     = PageStart + PageSize;
    if (PageEnd > RegionCount) {
      PageEnd = RegionCount;
    }
    ItemsInPage = PageEnd - PageStart;

    DisplayIndex = 0;
    RegionCount  = 0;

    PrintInfo (L"REGIONES DE MEMORIA:");
    Print (L"\n");

    for (Index = 0; Index < MemoryMapSize / DescriptorSize; Index++) {
      Desc = (EFI_MEMORY_DESCRIPTOR *)((UINT8 *)MemoryMap + Index * DescriptorSize);

      if ((Desc->Type == EfiConventionalMemory) ||
          (Desc->Type == EfiRuntimeServicesData) ||
          (Desc->Type == EfiBootServicesData))
      {
        if (RegionCount >= PageStart && RegionCount < PageEnd) {
          switch (Desc->Type) {
            case EfiRuntimeServicesData:
              TypeStr = L"Runtime Datos";
              break;
            case EfiBootServicesData:
              TypeStr = L"Boot Data";
              break;
            case EfiConventionalMemory:
              TypeStr = L"Convencional";
              break;
            default:
              TypeStr = L"Desconocida";
              break;
          }

          Print (L"  [%d] %s\n", DisplayIndex + 1, TypeStr);
          Print (L"      Direccion: 0x%Lx\n", Desc->PhysicalStart);
          Print (
            L"      Paginas: %Lu (%Lu MB)\n",
            Desc->NumberOfPages,
            (Desc->NumberOfPages * 4096) / (1024 * 1024)
            );
          Print (L"\n");
          DisplayIndex++;
        }
        RegionCount++;
      }
    }

    Print (L"\n  Pagina %d de %d  (Total: %d regiones)\n", CurrentPage + 1, TotalPages, RegionCount);

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

  FreePool (MemoryMap);
}

// ============================================
// CONFIGURACION DEL SISTEMA
// ============================================

VOID
EFIAPI
ShowUEFIInfo (
  VOID
  )
{
  ClearScreen ();
  PrintTitle (L"INFORMACION DEL UEFI");
  Print (L"\n");

  Print (L"  Version del Firmware: %d.%d\n", gST->Hdr.Revision >> 16, gST->Hdr.Revision & 0xFFFF);
  Print (L"  Fabricante: %s\n", gST->FirmwareVendor);
  Print (L"  Revision: %d\n\n", gST->FirmwareRevision);

  Pause ();
}

VOID
EFIAPI
ShowBootVariables (
  VOID
  )
{
  EFI_STATUS  Status;
  UINT16      *BootOrder;
  UINTN       BootOrderSize;
  UINT16      BootTimeout;
  UINTN       BootTimeoutSize;
  UINTN       Index;

  ClearScreen ();
  PrintTitle (L"VARIABLES DE ARRANQUE");
  Print (L"\n");

  // Leer BootOrder
  BootOrderSize = 0;
  Status = gRT->GetVariable (
                  L"BootOrder",
                  &gEfiGlobalVariableGuid,
                  NULL,
                  &BootOrderSize,
                  NULL
                  );

  if (Status == EFI_BUFFER_TOO_SMALL && BootOrderSize > 0) {
    BootOrder = AllocatePool (BootOrderSize);
    if (BootOrder != NULL) {
      Status = gRT->GetVariable (
                      L"BootOrder",
                      &gEfiGlobalVariableGuid,
                      NULL,
                      &BootOrderSize,
                      BootOrder
                      );

      if (!EFI_ERROR (Status)) {
        Print (L"  Boot Order: ");
        for (Index = 0; Index < (BootOrderSize / sizeof (UINT16)); Index++) {
          Print (L"Boot%04X ", BootOrder[Index]);
        }
        Print (L"\n\n");
      }
      FreePool (BootOrder);
    }
  }

  // Leer Timeout de arranque
  BootTimeoutSize = sizeof (UINT16);
  BootTimeout     = 0;
  Status = gRT->GetVariable (
                  L"Timeout",
                  &gEfiGlobalVariableGuid,
                  NULL,
                  &BootTimeoutSize,
                  &BootTimeout
                  );

  if (!EFI_ERROR (Status)) {
    Print (L"  Timeout de arranque: %d segundos\n\n", BootTimeout);
  } else {
    Print (L"  Timeout de arranque: No configurado\n\n");
  }

  Pause ();
}



VOID
EFIAPI
ListUEFIVariables (
  VOID
  )
{
  EFI_STATUS  Status;
  CHAR16      VariableName[256];
  EFI_GUID    VendorGuid;
  UINTN       VariableNameSize;
  UINTN       Count;
  CHAR16      **VariableList;
  UINTN       MaxVariables;
  UINTN       PageSize;
  UINTN       TotalPages;
  UINTN       CurrentPage;
  UINTN       PageStart;
  UINTN       PageEnd;
  UINTN       ItemsInPage;
  BOOLEAN     Navigating;
  EFI_INPUT_KEY Key;
  UINTN       EventIndex;
  UINTN       Index;

  ClearScreen ();
  PrintTitle (L"VARIABLES UEFI");

  MaxVariables = 500;
  VariableList = AllocateZeroPool (sizeof (CHAR16 *) * MaxVariables);

  if (VariableList == NULL) {
    PrintError (L"No se pudo reservar memoria");
    Pause ();
    return;
  }

  Count = 0;
  VariableName[0] = L'\0';
  VariableNameSize = sizeof (VariableName);

  Status = gRT->GetNextVariableName (&VariableNameSize, VariableName, &VendorGuid);

  while (!EFI_ERROR (Status) && (Count < MaxVariables)) {
    VariableList[Count] = AllocateZeroPool (sizeof (CHAR16) * 256);
    if (VariableList[Count] != NULL) {
      StrCpyS (VariableList[Count], 256, VariableName);
    }
    Count++;

    // ← NO resetear VariableName aquí
    VariableNameSize = sizeof (VariableName);
    Status = gRT->GetNextVariableName (&VariableNameSize, VariableName, &VendorGuid);
  }

  if (Count == 0) {
    PrintError (L"No se encontraron variables UEFI");
    Pause ();
    FreePool (VariableList);
    return;
  }

  PageSize    = 5;
  TotalPages  = (Count + PageSize - 1) / PageSize;
  CurrentPage = 0;
  Navigating  = TRUE;

  while (Navigating) {
    ClearScreen ();
    PrintTitle (L"VARIABLES UEFI");

    PageStart   = CurrentPage * PageSize;
    PageEnd     = PageStart + PageSize;
    if (PageEnd > Count) {
      PageEnd = Count;
    }
    ItemsInPage = PageEnd - PageStart;

    PrintInfo (L"VARIABLES GLOBALES:");
    Print (L"\n");

    for (Index = PageStart; Index < PageEnd; Index++) {
      Print (L"  [%d] %s\n", (UINT32)(Index + 1), VariableList[Index]);
    }

    Print (L"\n  Pagina %d de %d  (Total: %d variables)\n", CurrentPage + 1, TotalPages, Count);

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

  for (Index = 0; Index < Count; Index++) {
    if (VariableList[Index] != NULL) {
      FreePool (VariableList[Index]);
    }
  }
  FreePool (VariableList);
}

VOID
EFIAPI
ShowHardwareInfo (
  VOID
  )
{
  EFI_STATUS  Status;
  UINTN       MemoryMapSize;
  EFI_MEMORY_DESCRIPTOR  *MemoryMap;
  UINTN       MapKey;
  UINTN       DescriptorSize;
  UINT32      DescriptorVersion;
  UINTN       Index;
  UINT64      TotalMemory;

  ClearScreen ();
  PrintTitle (L"INFORMACION DE HARDWARE");
  Print (L"\n");

  MemoryMapSize = 0;
  MemoryMap     = NULL;
  TotalMemory   = 0;

  Status = gBS->GetMemoryMap (
                  &MemoryMapSize,
                  MemoryMap,
                  &MapKey,
                  &DescriptorSize,
                  &DescriptorVersion
                  );

  if (Status == EFI_BUFFER_TOO_SMALL) {
    MemoryMapSize += 2 * DescriptorSize;
    MemoryMap      = AllocatePool (MemoryMapSize);

    if (MemoryMap != NULL) {
      Status = gBS->GetMemoryMap (
                      &MemoryMapSize,
                      MemoryMap,
                      &MapKey,
                      &DescriptorSize,
                      &DescriptorVersion
                      );

      if (!EFI_ERROR (Status)) {
        for (Index = 0; Index < MemoryMapSize / DescriptorSize; Index++) {
          EFI_MEMORY_DESCRIPTOR  *Desc;
          Desc = (EFI_MEMORY_DESCRIPTOR *)((UINT8 *)MemoryMap + Index * DescriptorSize);
          TotalMemory += Desc->NumberOfPages * 4096;
        }

        Print (L"  RAM Total: %Lu MB\n\n", TotalMemory / (1024 * 1024));
      }

      FreePool (MemoryMap);
    }
  }

  Print (L"  Procesador: x86/x64\n");
  Print (L"  Modo: UEFI\n");
  Print (L"\n");

  Pause ();
}

VOID
EFIAPI
SystemConfiguration (
  VOID
  )
{
  BOOLEAN     Navigating;
  EFI_INPUT_KEY Key;
  UINTN       EventIndex;

  Navigating = TRUE;

  while (Navigating) {
    ClearScreen ();
    PrintTitle (L"CONFIGURACION DEL SISTEMA");
    Print (L"\n");
    PrintInfo (L"INFORMACION DEL FIRMWARE:");
    Print (L"\n");
    
    Print (L"  [1] Informacion del UEFI\n");
    Print (L"      - Version del firmware\n");
    Print (L"      - Fabricante\n");
    Print (L"      - Revision\n");
    Print (L"\n");
    Print (L"  [2] Variables de arranque\n");
    Print (L"      - Boot Order\n");
    Print (L"      - Timeout de arranque\n");
    Print (L"\n");
    Print (L"  [3] Variables UEFI\n");
    Print (L"      - Listar variables globales\n");
    Print (L"\n");
    Print (L"  [4] Informacion de Hardware\n");
    Print (L"      - RAM total\n");
    Print (L"      - Procesador\n");
    Print (L"\n");
    Print (L"  [0] Volver\n");
    Print (L"\n  Selecciona una opcion: ");

    gBS->WaitForEvent (1, &gST->ConIn->WaitForKey, &EventIndex);
    gST->ConIn->ReadKeyStroke (gST->ConIn, &Key);

    switch (Key.UnicodeChar) {
      case L'1':
        ShowUEFIInfo ();
        break;
      case L'2':
        ShowBootVariables ();
        break;
        break;
      case L'3':
        ListUEFIVariables ();
        break;
      case L'4':
        ShowHardwareInfo ();
        break;
      case L'0':
        Navigating = FALSE;
        break;
      default:
        break;
    }
  }
}

/*

Funcionalidades

*/

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

  Print (L"  [1] Estado del Secure Boot\n");
  Print (L"  [2] Informacion del firmware\n");
  Print (L"  [3] Dump de memoria\n");
  Print (L"  [4] Configuracion del sistema\n");
  Print (L"  [0] Reiniciar\n\n");

  Print (L"----------------------------------------------------\n");
  Print (L"  Selecciona una opcion (0-9): ");
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

// ============================================
// FUNCION PRINCIPAL
// ============================================

EFI_STATUS
EFIAPI
UefiMain (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
{
  INT32    Choice;
  BOOLEAN  ExitApp;

  gImageHandle = ImageHandle;
  Create_directory (gImageHandle);

  gST->ConOut->EnableCursor (gST->ConOut, FALSE);

  ExitApp = FALSE;

  while (!ExitApp) {
    ShowMainMenu ();
    Choice = GetMenuChoice ();

    switch (Choice) {
      case 1:
        CheckSecureBoot ();
        break;
      case 2:
        ShowFirmwareInfo ();
        break;
      case 3:
        DumpMemory ();
        break;
      case 4:
        SystemConfiguration ();
        break;
      case 0:
        Print (L"\nReiniciando sistema...\n");
        ExitApp = TRUE;
        ClearScreen ();
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
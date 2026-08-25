

#include "Forense_tool.h"

#include <Library/UefiApplicationEntryPoint.h>
#include <Library/UefiLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/UefiRuntimeServicesTableLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/BaseLib.h>
#include <Library/PrintLib.h>
#include <Library/BaseCryptLib.h>
#include <Protocol/Tcg2Protocol.h>

/*==========================================================================
  CONSOLA (mismo patron que el resto de modulos)
==========================================================================*/

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
  Print (L"      FORENSIC EXTRA - REPORTING / NVRAM / GPT / TPM\n");
  Print (L"====================================================\n\n");

  gST->ConOut->SetAttribute (gST->ConOut, COLOR_MENU);
  Print (L"----------------------------------------------------\n");
  Print (L"  MENU\n");
  Print (L"----------------------------------------------------\n\n");

  Print (L"  [1] Generar reporte forense (.efi + hash + log)\n");
  Print (L"  [2] Comparar hashes contra whitelist\n");
  Print (L"  [3] Variables de arranque (BootXXXX / BootOrder)\n");
  Print (L"  [4] Variables Secure Boot (PK/KEK/db/dbx)\n");
  Print (L"  [5] Tabla de particiones GPT\n");
  Print (L"  [6] TPM2 - Lectura de PCRs\n");
  Print (L"  [0] Volver\n\n");

  Print (L"----------------------------------------------------\n");
  Print (L"  Selecciona una opcion (0-6): ");
}

/*==========================================================================
  UTILIDADES INTERNAS
==========================================================================*/

STATIC
VOID
HashToHexStringA (
  IN  CONST UINT8  *Hash,
  IN  UINTN         HashSize,
  OUT CHAR8        *OutBuffer  // debe tener espacio para HashSize*2 + 1
  )
{
  CONST CHAR8  *HexDigits = "0123456789abcdef";
  UINTN        i;

  for (i = 0; i < HashSize; i++) {
    OutBuffer[i * 2]     = HexDigits[(Hash[i] >> 4) & 0xF];
    OutBuffer[i * 2 + 1] = HexDigits[Hash[i] & 0xF];
  }
  OutBuffer[HashSize * 2] = '\0';
}

STATIC
VOID
GetTimestampA (
  OUT CHAR8   *OutBuffer,
  IN  UINTN   BufferSize
  )
{
  EFI_TIME  Time;

  if (EFI_ERROR (gRT->GetTime (&Time, NULL))) {
    AsciiSPrint (OutBuffer, BufferSize, "0000-00-00 00:00:00");
    return;
  }

  AsciiSPrint (
    OutBuffer,
    BufferSize,
    "%04d-%02d-%02d %02d:%02d:%02d",
    Time.Year,
    Time.Month,
    Time.Day,
    Time.Hour,
    Time.Minute,
    Time.Second
    );
}

//
// Obtiene el device handle del propio binario (medio de arranque de la
// herramienta), NUNCA el disco que se esta analizando.
//
STATIC
EFI_STATUS
GetOwnDeviceHandle (
  OUT EFI_HANDLE  *DeviceHandle
  )
{
  EFI_STATUS                  Status;
  EFI_LOADED_IMAGE_PROTOCOL   *LoadedImage;

  Status = gBS->HandleProtocol (
                  gImageHandle,
                  &gEfiLoadedImageProtocolGuid,
                  (VOID **)&LoadedImage
                  );

  if (EFI_ERROR (Status)) {
    return Status;
  }

  *DeviceHandle = LoadedImage->DeviceHandle;
  return EFI_SUCCESS;
}

//
// Abre (o crea si no existe) el fichero de log en el propio medio de
// arranque y posiciona el cursor al final para hacer append.
//
STATIC
EFI_STATUS
OpenOwnLogForAppend (
  IN  CONST CHAR16       *FileName,
  OUT EFI_FILE_PROTOCOL  **OutFile
  )
{
  EFI_STATUS                        Status;
  EFI_HANDLE                        DeviceHandle;
  EFI_SIMPLE_FILE_SYSTEM_PROTOCOL   *FileSystem;
  EFI_FILE_PROTOCOL                 *Root;
  EFI_FILE_PROTOCOL                 *File;

  Status = GetOwnDeviceHandle (&DeviceHandle);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  Status = gBS->HandleProtocol (
                  DeviceHandle,
                  &gEfiSimpleFileSystemProtocolGuid,
                  (VOID **)&FileSystem
                  );
  if (EFI_ERROR (Status)) {
    return Status;
  }

  Status = FileSystem->OpenVolume (FileSystem, &Root);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  Status = Root->Open (
                   Root,
                   &File,
                   (CHAR16 *)FileName,
                   EFI_FILE_MODE_READ | EFI_FILE_MODE_WRITE | EFI_FILE_MODE_CREATE,
                   0
                   );

  Root->Close (Root);

  if (EFI_ERROR (Status)) {
    return Status;
  }

  //
  // 0xFFFFFFFFFFFFFFFF como posicion es la forma estandar de UEFI de
  // decir "coloca el cursor al final del fichero" (append).
  //
  File->SetPosition (File, 0xFFFFFFFFFFFFFFFFULL);

  *OutFile = File;
  return EFI_SUCCESS;
}

STATIC
VOID
AppendLogLineA (
  IN CONST CHAR8  *Line
  )
{
  EFI_STATUS          Status;
  EFI_FILE_PROTOCOL   *File;
  UINTN                Len;

  Status = OpenOwnLogForAppend (LOG_FILE_NAME, &File);
  if (EFI_ERROR (Status)) {
    PrintError (L"No se pudo abrir el log en el medio de arranque: %r", Status);
    return;
  }

  Len = AsciiStrLen (Line);
  File->Write (File, &Len, (VOID *)Line);

  //
  // Salto de linea aparte para no depender de que el llamador lo incluya.
  //
  Len = 2;
  File->Write (File, &Len, (VOID *)"\r\n");

  File->Close (File);
}

/*==========================================================================
  ESCANEO DE ARCHIVOS .EFI (auto-contenido, con hash opcional)
==========================================================================*/

STATIC
VOID
ScanDirectoryRecursive (
  IN     EFI_FILE_PROTOCOL  *Dir,
  IN     EFI_HANDLE         DeviceHandle,
  IN     CONST CHAR16       *CurrentPath,
  IN     UINTN              Depth,
  IN OUT EFI_FILE_ENTRY     *Entries,
  IN OUT UINTN              *Found,
  IN     UINTN              MaxEntries,
  IN     BOOLEAN            ComputeHash
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
    if (EFI_ERROR (Status) || (BufferSize == 0)) {
      FreePool (FileInfo);
      break;
    }

    if ((StrCmp (FileInfo->FileName, L".") == 0) ||
        (StrCmp (FileInfo->FileName, L"..") == 0))
    {
      FreePool (FileInfo);
      continue;
    }

    if (StrCmp (CurrentPath, L"\\") == 0) {
      UnicodeSPrint (NewPath, sizeof (NewPath), L"\\%s", FileInfo->FileName);
    } else {
      UnicodeSPrint (NewPath, sizeof (NewPath), L"%s\\%s", CurrentPath, FileInfo->FileName);
    }

    if (FileInfo->Attribute & EFI_FILE_DIRECTORY) {
      if (Depth < MAX_SCAN_DEPTH) {
        EFI_FILE_PROTOCOL  *SubDir = NULL;

        Status = Dir->Open (Dir, &SubDir, FileInfo->FileName, EFI_FILE_MODE_READ, 0);
        if (!EFI_ERROR (Status)) {
          ScanDirectoryRecursive (
            SubDir,
            DeviceHandle,
            NewPath,
            Depth + 1,
            Entries,
            Found,
            MaxEntries,
            ComputeHash
            );
          SubDir->Close (SubDir);
        }
      }
    } else if (StrStr (FileInfo->FileName, L".efi") != NULL) {
      Entries[*Found].DeviceHandle = DeviceHandle;
      Entries[*Found].FileSize     = FileInfo->FileSize;
      Entries[*Found].HashValid    = FALSE;
      StrCpyS (Entries[*Found].FilePath, ARRAY_SIZE (Entries[*Found].FilePath), NewPath);

      if (ComputeHash && (FileInfo->FileSize > 0)) {
        EFI_FILE_PROTOCOL  *EfiFile = NULL;

        Status = Dir->Open (Dir, &EfiFile, FileInfo->FileName, EFI_FILE_MODE_READ, 0);
        if (!EFI_ERROR (Status)) {
          UINTN  ReadSize;
          VOID   *FileBuffer;

          ReadSize   = (UINTN)FileInfo->FileSize;
          FileBuffer = AllocatePool (ReadSize);

          if (FileBuffer != NULL) {
            Status = EfiFile->Read (EfiFile, &ReadSize, FileBuffer);
            if (!EFI_ERROR (Status)) {
              if (Sha256HashAll (FileBuffer, ReadSize, Entries[*Found].Hash)) {
                Entries[*Found].HashValid = TRUE;
              }
            }
            FreePool (FileBuffer);
          }

          EfiFile->Close (EfiFile);
        }
      }

      (*Found)++;
    }

    FreePool (FileInfo);
  }
}

UINTN
EFIAPI
FindEfiFiles (
  IN OUT EFI_FILE_ENTRY  *Entries,
  IN     UINTN            MaxEntries,
  IN     BOOLEAN          ComputeHash
  )
{
  EFI_STATUS  Status;
  UINTN       HandleCount;
  EFI_HANDLE  *HandleBuffer;
  UINTN       HandleIndex;
  UINTN       Found;

  HandleCount  = 0;
  HandleBuffer = NULL;
  Found        = 0;

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

    FileSystem = NULL;
    Root       = NULL;

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
      MaxEntries,
      ComputeHash
      );

    Root->Close (Root);
  }

  FreePool (HandleBuffer);
  return Found;
}

/*==========================================================================
  1) REPORTE FORENSE (persistencia con hash + timestamp)
==========================================================================*/

VOID
EFIAPI
GenerateForensicReport (
  VOID
  )
{
  EFI_FILE_ENTRY  *Entries;
  UINTN           Count;
  UINTN           Index;
  CHAR8           TimeStamp[32];
  CHAR8           HashHex[65];
  CHAR8           LogLine[MAX_FILE_PATH_LEN + 160];
  CHAR8           HeaderLine[128];

  ClearScreen ();
  PrintTitle (L"GENERAR REPORTE FORENSE");

  PrintInfo (L"Escaneando volumenes y calculando SHA-256 (puede tardar)...");
  Print (L"\n");

  Entries = AllocateZeroPool (sizeof (EFI_FILE_ENTRY) * MAX_FILE_ENTRIES);
  if (Entries == NULL) {
    PrintError (L"No se pudo reservar memoria");
    Pause ();
    return;
  }

  Count = FindEfiFiles (Entries, MAX_FILE_ENTRIES, TRUE);

  if (Count == 0) {
    PrintError (L"No se encontraron archivos .efi");
    Pause ();
    FreePool (Entries);
    return;
  }

  GetTimestampA (TimeStamp, sizeof (TimeStamp));
  AsciiSPrint (
    HeaderLine,
    sizeof (HeaderLine),
    "==== REPORTE FORENSE %a ====",
    TimeStamp
    );
  AppendLogLineA (HeaderLine);

  for (Index = 0; Index < Count; Index++) {
    CHAR8  PathA[MAX_FILE_PATH_LEN];

    UnicodeStrToAsciiStrS (Entries[Index].FilePath, PathA, sizeof (PathA));

    if (Entries[Index].HashValid) {
      HashToHexStringA (Entries[Index].Hash, 32, HashHex);
    } else {
      AsciiSPrint (HashHex, sizeof (HashHex), "ERROR_AL_CALCULAR_HASH");
    }

    Print (L"  [%d] %s\n", (UINT32)(Index + 1), Entries[Index].FilePath);
    Print (L"      Tamano: %Lu bytes\n", Entries[Index].FileSize);
    Print (L"      Hash:   %a\n\n", HashHex);

    AsciiSPrint (
      LogLine,
      sizeof (LogLine),
      "[%a] PATH=%a SIZE=%Lu HASH=%a",
      TimeStamp,
      PathA,
      Entries[Index].FileSize,
      HashHex
      );
    AppendLogLineA (LogLine);
  }

  PrintSuccess (L"Reporte anadido a %s (medio de arranque de la herramienta)", LOG_FILE_NAME);
  Pause ();
  FreePool (Entries);
}

/*==========================================================================
  2) COMPARAR CONTRA WHITELIST
==========================================================================*/

//
// Formato esperado de whitelist.txt: un hash SHA-256 en hex por linea
// (64 caracteres), lineas vacias o que no midan 64 caracteres se ignoran.
//
STATIC
EFI_STATUS
LoadWhitelist (
  OUT CHAR8   **HashList,   // array de punteros a strings de 64 chars + NUL
  OUT UINTN   *HashCount
  )
{
  EFI_STATUS           Status;
  EFI_HANDLE           DeviceHandle;
  EFI_SIMPLE_FILE_SYSTEM_PROTOCOL  *FileSystem;
  EFI_FILE_PROTOCOL    *Root;
  EFI_FILE_PROTOCOL    *File;
  EFI_FILE_INFO        *FileInfo;
  UINTN                InfoSize;
  UINTN                FileSize;
  CHAR8                *RawBuffer;
  UINTN                ReadSize;
  UINTN                i, LineStart, Count, MaxLines;
  CHAR8                **List;

  *HashList  = NULL;
  *HashCount = 0;

  Status = GetOwnDeviceHandle (&DeviceHandle);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  Status = gBS->HandleProtocol (DeviceHandle, &gEfiSimpleFileSystemProtocolGuid, (VOID **)&FileSystem);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  Status = FileSystem->OpenVolume (FileSystem, &Root);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  Status = Root->Open (Root, &File, WHITELIST_FILE_NAME, EFI_FILE_MODE_READ, 0);
  Root->Close (Root);

  if (EFI_ERROR (Status)) {
    return Status;   // normalmente EFI_NOT_FOUND: no existe whitelist
  }

  InfoSize = SIZE_OF_EFI_FILE_INFO + 256;
  FileInfo = AllocatePool (InfoSize);
  if (FileInfo == NULL) {
    File->Close (File);
    return EFI_OUT_OF_RESOURCES;
  }

  Status = File->GetInfo (File, &gEfiFileInfoGuid, &InfoSize, FileInfo);
  if (EFI_ERROR (Status)) {
    FreePool (FileInfo);
    File->Close (File);
    return Status;
  }

  FileSize = (UINTN)FileInfo->FileSize;
  FreePool (FileInfo);

  if (FileSize == 0) {
    File->Close (File);
    return EFI_NOT_FOUND;
  }

  RawBuffer = AllocatePool (FileSize + 1);
  if (RawBuffer == NULL) {
    File->Close (File);
    return EFI_OUT_OF_RESOURCES;
  }

  ReadSize = FileSize;
  Status   = File->Read (File, &ReadSize, RawBuffer);
  File->Close (File);

  if (EFI_ERROR (Status)) {
    FreePool (RawBuffer);
    return Status;
  }
  RawBuffer[ReadSize] = '\0';

  //
  // Reserva conservadora: una linea de hash ocupa al menos 64 caracteres,
  // asi que el numero de lineas nunca supera ReadSize/64 + 1.
  //
  MaxLines = (ReadSize / 64) + 1;
  List     = AllocateZeroPool (sizeof (CHAR8 *) * MaxLines);
  if (List == NULL) {
    FreePool (RawBuffer);
    return EFI_OUT_OF_RESOURCES;
  }

  Count     = 0;
  LineStart = 0;

  for (i = 0; i <= ReadSize; i++) {
    if ((i == ReadSize) || (RawBuffer[i] == '\n') || (RawBuffer[i] == '\r')) {
      UINTN  LineLen = i - LineStart;

      if ((LineLen == 64) && (Count < MaxLines)) {
        List[Count] = AllocatePool (65);
        if (List[Count] != NULL) {
          CopyMem (List[Count], &RawBuffer[LineStart], 64);
          List[Count][64] = '\0';
          Count++;
        }
      }
      LineStart = i + 1;
    }
  }

  FreePool (RawBuffer);

  *HashList  = (CHAR8 *)List;   // nota: en realidad es CHAR8** , ver uso abajo
  *HashCount = Count;

  return EFI_SUCCESS;
}

VOID
EFIAPI
CompareAgainstWhitelist (
  VOID
  )
{
  EFI_STATUS      Status;
  CHAR8           **WhiteList;
  UINTN           WhiteCount;
  EFI_FILE_ENTRY  *Entries;
  UINTN           Count;
  UINTN           Index;
  UINTN           w;
  CHAR8           HashHex[65];
  CHAR8           TimeStamp[32];
  CHAR8           PathA[MAX_FILE_PATH_LEN];
  CHAR8           LogLine[MAX_FILE_PATH_LEN + 160];

  ClearScreen ();
  PrintTitle (L"COMPARAR CONTRA WHITELIST");

  WhiteList = NULL;
  WhiteCount = 0;

  // ← CAST EXPLÍCITO: (CHAR8 **)
  Status = LoadWhitelist ((CHAR8 **)&WhiteList, &WhiteCount);
  if (EFI_ERROR (Status) || (WhiteCount == 0)) {
    PrintError (L"No se pudo cargar la whitelist (%s)", WHITELIST_FILE_NAME);
    PrintInfo (L"Crea ese fichero en el medio de arranque de la herramienta,");
    PrintInfo (L"con un hash SHA-256 (64 caracteres hex) por linea.");
    Pause ();
    return;
  }

  PrintSuccess (L"Whitelist cargada: %d hashes conocidos", WhiteCount);
  Print (L"\n");

  Entries = AllocateZeroPool (sizeof (EFI_FILE_ENTRY) * MAX_FILE_ENTRIES);
  if (Entries == NULL) {
    PrintError (L"No se pudo reservar memoria");
    Pause ();
    goto Cleanup;
  }

  PrintInfo (L"Escaneando volumenes y calculando SHA-256...");
  Print (L"\n");

  Count = FindEfiFiles (Entries, MAX_FILE_ENTRIES, TRUE);
  if (Count == 0) {
    PrintError (L"No se encontraron archivos .efi");
    Pause ();
    FreePool (Entries);
    goto Cleanup;
  }

  GetTimestampA (TimeStamp, sizeof (TimeStamp));
  Print (L"\n");

  for (Index = 0; Index < Count; Index++) {
    BOOLEAN  Known;

    Known = FALSE;

    if (Entries[Index].HashValid) {
      HashToHexStringA (Entries[Index].Hash, 32, HashHex);

      for (w = 0; w < WhiteCount; w++) {
        if (WhiteList[w] != NULL) {
          if (AsciiStriCmp (HashHex, WhiteList[w]) == 0) {
            Known = TRUE;
            break;
          }
        }
      }
    } else {
      AsciiSPrint (HashHex, sizeof (HashHex), "ERROR_AL_CALCULAR_HASH");
    }

    Print (L"  %s\n", Entries[Index].FilePath);
    Print (L"      Tamano: %Lu bytes\n", Entries[Index].FileSize);
    Print (L"      Hash: %a\n", HashHex);

    if (Known) {
      PrintSuccess (L"      COINCIDE CON WHITELIST (archivo conocido)");
    } else {
      PrintError (L"      NO ESTA EN WHITELIST (archivo desconocido, revisar)");
    }
    Print (L"\n");

    // Registrar en log
    UnicodeStrToAsciiStrS (Entries[Index].FilePath, PathA, sizeof (PathA));
    AsciiSPrint (
      LogLine,
      sizeof (LogLine),
      "[%a] WHITELIST_CHECK PATH=%a HASH=%a RESULT=%a",
      TimeStamp,
      PathA,
      HashHex,
      Known ? "CONOCIDO" : "DESCONOCIDO"
      );
    AppendLogLineA (LogLine);
  }

  PrintSuccess (L"Comparacion completada. Resultados guardados en %s", LOG_FILE_NAME);
  Pause ();
  FreePool (Entries);

Cleanup:
  if (WhiteList != NULL) {
    for (w = 0; w < WhiteCount; w++) {
      if (WhiteList[w] != NULL) {
        FreePool (WhiteList[w]);
      }
    }
    FreePool (WhiteList);
  }
}

/*==========================================================================
  3) VARIABLES DE ARRANQUE (BootXXXX / BootOrder)
==========================================================================*/

STATIC
BOOLEAN
IsHexChar (
  IN CHAR16  c
  )
{
  return ((c >= L'0') && (c <= L'9')) ||
         ((c >= L'A') && (c <= L'F')) ||
         ((c >= L'a') && (c <= L'f'));
}

VOID
EFIAPI
ShowBootVariables (
  VOID
  )
{
  EFI_STATUS  Status;
  UINTN       DataSize;
  UINT16      *BootOrder;
  UINTN       OrderCount;
  UINTN       i;
  CHAR16      VarName[512];
  UINTN       VarNameSize;
  EFI_GUID    VendorGuid;

  ClearScreen ();
  PrintTitle (L"VARIABLES DE ARRANQUE (NVRAM)");

  //
  // BootOrder
  //
  DataSize  = 0;
  BootOrder = NULL;
  Status    = gRT->GetVariable (L"BootOrder", &gEfiGlobalVariableGuid, NULL, &DataSize, NULL);

  if (Status == EFI_BUFFER_TOO_SMALL) {
    BootOrder = AllocatePool (DataSize);
    if (BootOrder != NULL) {
      Status = gRT->GetVariable (L"BootOrder", &gEfiGlobalVariableGuid, NULL, &DataSize, BootOrder);
    }
  }

  if ((BootOrder != NULL) && !EFI_ERROR (Status)) {
    OrderCount = DataSize / sizeof (UINT16);
    PrintInfo (L"BootOrder (%d entradas):", OrderCount);
    Print (L"  ");
    for (i = 0; i < OrderCount; i++) {
      Print (L"Boot%04x  ", BootOrder[i]);
    }
    Print (L"\n\n");
    FreePool (BootOrder);
  } else {
    PrintError (L"No se pudo leer BootOrder");
  }

  //
  // Enumerar todas las variables globales y filtrar BootXXXX
  //
  PrintInfo (L"Entradas de arranque (BootXXXX):");
  Print (L"\n");

  VarName[0] = L'\0';
  ZeroMem (&VendorGuid, sizeof (VendorGuid));

  for ( ; ; ) {
    VarNameSize = sizeof (VarName);
    Status = gRT->GetNextVariableName (&VarNameSize, VarName, &VendorGuid);

    if (Status == EFI_NOT_FOUND) {
      break;
    }
    if (EFI_ERROR (Status)) {
      break;
    }

    if (CompareGuid (&VendorGuid, &gEfiGlobalVariableGuid) &&
        (StrLen (VarName) == 8) &&
        (StrnCmp (VarName, L"Boot", 4) == 0) &&
        IsHexChar (VarName[4]) && IsHexChar (VarName[5]) &&
        IsHexChar (VarName[6]) && IsHexChar (VarName[7]))
    {
      UINTN  LoadOptSize;
      UINT8  *LoadOpt;

      LoadOptSize = 0;
      Status = gRT->GetVariable (VarName, &VendorGuid, NULL, &LoadOptSize, NULL);

      if (Status == EFI_BUFFER_TOO_SMALL) {
        LoadOpt = AllocatePool (LoadOptSize);
        if (LoadOpt != NULL) {
          Status = gRT->GetVariable (VarName, &VendorGuid, NULL, &LoadOptSize, LoadOpt);

          if (!EFI_ERROR (Status) && (LoadOptSize >= 6)) {
            UINT32  Attributes;
            CHAR16  *Description;

            Attributes  = *(UINT32 *)LoadOpt;
            // Description (CHAR16, NUL-terminado) empieza en el offset 6,
            // segun EFI_LOAD_OPTION (UEFI Spec 3.1.3).
            Description = (CHAR16 *)(LoadOpt + 6);

            Print (
              L"  [%s] %s%s\n",
              VarName,
              (Attributes & 0x00000001) ? L"[ACTIVA]  " : L"[inactiva] ",
              Description
              );
          }

          FreePool (LoadOpt);
        }
      }
    }
  }

  Print (L"\n");
  Pause ();
}

/*==========================================================================
  4) VARIABLES SECURE BOOT (PK / KEK / db / dbx)
==========================================================================*/

STATIC
VOID
ShowSecureBootVar (
  IN CONST CHAR16  *Name,
  IN EFI_GUID       *Guid
  )
{
  EFI_STATUS  Status;
  UINTN       DataSize;

  DataSize = 0;
  Status   = gRT->GetVariable ((CHAR16 *)Name, Guid, NULL, &DataSize, NULL);

  if (Status == EFI_BUFFER_TOO_SMALL) {
    PrintSuccess (L"%-6s definida  (%d bytes)", Name, DataSize);
  } else if (Status == EFI_NOT_FOUND) {
    PrintError (L"%-6s NO definida", Name);
  } else {
    PrintInfo (L"%-6s estado desconocido (%r)", Name, Status);
  }
}

VOID
EFIAPI
ShowSecureBootVariables (
  VOID
  )
{
  ClearScreen ();
  PrintTitle (L"VARIABLES SECURE BOOT");

  Print (L"\n");
  ShowSecureBootVar (L"PK", &gEfiGlobalVariableGuid);
  ShowSecureBootVar (L"KEK", &gEfiGlobalVariableGuid);
  ShowSecureBootVar (L"db", &gEfiImageSecurityDatabaseGuid);
  ShowSecureBootVar (L"dbx", &gEfiImageSecurityDatabaseGuid);

  Print (L"\n");
  PrintInfo (L"PK ausente = firmware en 'Setup Mode' (Secure Boot no aplicable)");
  PrintInfo (L"db/dbx ausentes = no hay listas de firmas configuradas");

  Pause ();
}

/*==========================================================================
  5) TABLA DE PARTICIONES GPT
==========================================================================*/

STATIC CONST EFI_GUID  mEspTypeGuid = {
  0xC12A7328, 0xF81F, 0x11D2, { 0xBA, 0x4B, 0x00, 0xA0, 0xC9, 0x3E, 0xC9, 0x3B }
};

VOID
EFIAPI
ShowGptPartitions (
  VOID
  )
{
  EFI_STATUS  Status;
  UINTN       HandleCount;
  EFI_HANDLE  *HandleBuffer;
  UINTN       Index;
  UINTN       DiskNum;

  ClearScreen ();
  PrintTitle (L"TABLA DE PARTICIONES GPT");

  Status = gBS->LocateHandleBuffer (
                  ByProtocol,
                  &gEfiBlockIoProtocolGuid,
                  NULL,
                  &HandleCount,
                  &HandleBuffer
                  );

  if (EFI_ERROR (Status)) {
    PrintError (L"No se encontraron dispositivos de bloque");
    Pause ();
    return;
  }

  DiskNum = 1;

  for (Index = 0; Index < HandleCount; Index++) {
    EFI_BLOCK_IO_PROTOCOL       *BlockIo;
    VOID                         *HeaderBuf;
    UINTN                        BlockSize;
    EFI_PARTITION_TABLE_HEADER  *GptHeader;

    Status = gBS->HandleProtocol (HandleBuffer[Index], &gEfiBlockIoProtocolGuid, (VOID **)&BlockIo);
    if (EFI_ERROR (Status) || BlockIo->Media->LogicalPartition) {
      continue;   // solo discos completos, no particiones ya montadas
    }

    BlockSize = BlockIo->Media->BlockSize;
    if (BlockSize < sizeof (EFI_PARTITION_TABLE_HEADER)) {
      BlockSize = sizeof (EFI_PARTITION_TABLE_HEADER);
    }

    HeaderBuf = AllocatePool (BlockSize);
    if (HeaderBuf == NULL) {
      continue;
    }

    Status = BlockIo->ReadBlocks (BlockIo, BlockIo->Media->MediaId, 1, BlockSize, HeaderBuf);

    Print (L"\n  --- Disco %d (Handle %p) ---\n", DiskNum++, HandleBuffer[Index]);

    if (EFI_ERROR (Status)) {
      PrintError (L"      Error leyendo LBA1: %r", Status);
      FreePool (HeaderBuf);
      continue;
    }

    GptHeader = (EFI_PARTITION_TABLE_HEADER *)HeaderBuf;

    if (GptHeader->Header.Signature != EFI_PTAB_HEADER_ID) {
      PrintInfo (L"      No es GPT (posible MBR o disco vacio)");
      FreePool (HeaderBuf);
      continue;
    }

    PrintSuccess (L"      Cabecera GPT valida");
    Print (L"      Entradas de particion: %d\n", GptHeader->NumberOfPartitionEntries);
    Print (L"      Tamano por entrada:    %d bytes\n", GptHeader->SizeOfPartitionEntry);
    Print (L"      LBA de entradas:       %Lu\n\n", GptHeader->PartitionEntryLBA);

    {
      UINTN  EntriesBytes;
      UINTN  EntriesBlocks;
      VOID   *EntriesBuf;
      UINTN  e;

      EntriesBytes  = GptHeader->NumberOfPartitionEntries * GptHeader->SizeOfPartitionEntry;
      EntriesBlocks = (EntriesBytes + BlockIo->Media->BlockSize - 1) / BlockIo->Media->BlockSize;

      EntriesBuf = AllocatePool (EntriesBlocks * BlockIo->Media->BlockSize);
      if (EntriesBuf == NULL) {
        FreePool (HeaderBuf);
        continue;
      }

      Status = BlockIo->ReadBlocks (
                          BlockIo,
                          BlockIo->Media->MediaId,
                          GptHeader->PartitionEntryLBA,
                          EntriesBlocks * BlockIo->Media->BlockSize,
                          EntriesBuf
                          );

      if (!EFI_ERROR (Status)) {
        for (e = 0; e < GptHeader->NumberOfPartitionEntries; e++) {
          EFI_PARTITION_ENTRY  *Entry;
          UINT64                SizeMb;
          BOOLEAN               IsZero;
          UINTN                 z;

          Entry = (EFI_PARTITION_ENTRY *)((UINT8 *)EntriesBuf + e * GptHeader->SizeOfPartitionEntry);

          IsZero = TRUE;
          for (z = 0; z < sizeof (EFI_GUID); z++) {
            if (((UINT8 *)&Entry->PartitionTypeGUID)[z] != 0) {
              IsZero = FALSE;
              break;
            }
          }
          if (IsZero) {
            continue;   // slot vacio
          }

          SizeMb = ((Entry->EndingLBA - Entry->StartingLBA + 1) * BlockIo->Media->BlockSize) / (1024 * 1024);

          Print (L"        [%d] %s\n", (UINT32)(e + 1), Entry->PartitionName);
          Print (L"            LBA %Lu - %Lu  (~%Lu MB)\n", Entry->StartingLBA, Entry->EndingLBA, SizeMb);

          if (CompareGuid (&Entry->PartitionTypeGUID, &mEspTypeGuid)) {
            PrintSuccess (L"            Tipo: EFI System Partition (ESP)");
          }
        }
      } else {
        PrintError (L"      Error leyendo entradas de particion: %r", Status);
      }

      FreePool (EntriesBuf);
    }

    FreePool (HeaderBuf);
  }

  FreePool (HandleBuffer);
  Print (L"\n");
  Pause ();
}

/*==========================================================================
  6) TPM2 - LECTURA DE PCRs
  Construccion manual del comando TPM2_PCR_Read (TCG TPM2 Library Spec,
  Part 3: Commands) enviado via EFI_TCG2_PROTOCOL->SubmitCommand.
  ADVERTENCIA: parte avanzada, probar en OVMF+swtpm antes de usarla como
  evidencia real.
==========================================================================*/

//
// NOTA: con prefijo FX_ a proposito. IndustryStandard/Tpm20.h (incluido
// transitivamente por Protocol/Tcg2Protocol.h) ya define TPM_ALG_SHA256,
// TPM_RC_SUCCESS y TPM_ST_NO_SESSIONS con estos mismos valores; usar esos
// nombres aqui provoca un warning de redefinicion (C4005) que en tu build
// esta configurado como error (/WX). Prefijamos para no chocar con ellos.
//
#define FX_TPM_ST_NO_SESSIONS  0x8001
#define FX_TPM_CC_PCR_READ     0x0000017E
#define FX_TPM_ALG_SHA256      0x000B
#define FX_TPM_RC_SUCCESS      0x00000000

STATIC VOID WriteBE16 (UINT8 *Buf, UINT16 Val) { Buf[0] = (UINT8)(Val >> 8); Buf[1] = (UINT8)Val; }
STATIC VOID WriteBE32 (UINT8 *Buf, UINT32 Val) { Buf[0] = (UINT8)(Val >> 24); Buf[1] = (UINT8)(Val >> 16); Buf[2] = (UINT8)(Val >> 8); Buf[3] = (UINT8)Val; }
STATIC UINT16 ReadBE16 (CONST UINT8 *Buf) { return (UINT16)(((UINT16)Buf[0] << 8) | Buf[1]); }
STATIC UINT32 ReadBE32 (CONST UINT8 *Buf) { return ((UINT32)Buf[0] << 24) | ((UINT32)Buf[1] << 16) | ((UINT32)Buf[2] << 8) | Buf[3]; }

STATIC
EFI_STATUS
Tpm2ReadPcrSha256 (
  IN  EFI_TCG2_PROTOCOL  *Tcg2,
  IN  UINT8               PcrIndex,
  OUT UINT8               Digest[32],
  OUT BOOLEAN             *DigestPresent
  )
{
  UINT8       Cmd[32];
  UINT8       Resp[512];
  UINT32      CmdSize;
  EFI_STATUS  Status;
  UINT32      RespCode;
  UINTN       Offset;
  UINT32      SelCount;
  UINT32      i;
  UINT32      DigestCount;

  *DigestPresent = FALSE;
  ZeroMem (Cmd, sizeof (Cmd));

  WriteBE16 (&Cmd[0], FX_TPM_ST_NO_SESSIONS);
  // Cmd[2..5] = commandSize, se rellena al final
  WriteBE32 (&Cmd[6], FX_TPM_CC_PCR_READ);

  // TPML_PCR_SELECTION: count = 1
  WriteBE32 (&Cmd[10], 1);

  // TPMS_PCR_SELECTION: hash = SHA256, sizeofSelect = 3, pcrSelect[3]
  WriteBE16 (&Cmd[14], FX_TPM_ALG_SHA256);
  Cmd[16] = 3;
  Cmd[17] = 0;
  Cmd[18] = 0;
  Cmd[19] = 0;
  Cmd[17 + (PcrIndex / 8)] |= (UINT8)(1 << (PcrIndex % 8));

  CmdSize = 20;
  WriteBE32 (&Cmd[2], CmdSize);

  ZeroMem (Resp, sizeof (Resp));

  Status = Tcg2->SubmitCommand (Tcg2, CmdSize, Cmd, sizeof (Resp), Resp);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  RespCode = ReadBE32 (&Resp[6]);
  if (RespCode != FX_TPM_RC_SUCCESS) {
    return EFI_DEVICE_ERROR;
  }

  // Respuesta: tag(2) size(4) code(4) pcrUpdateCounter(4) pcrSelOut...
  Offset = 10 + 4;   // saltar cabecera + pcrUpdateCounter

  SelCount = ReadBE32 (&Resp[Offset]);
  Offset  += 4;

  for (i = 0; i < SelCount; i++) {
    UINT8  SizeOfSelect;

    Offset      += 2;                 // hashAlg
    SizeOfSelect = Resp[Offset];
    Offset      += 1 + SizeOfSelect;  // sizeofSelect + pcrSelect[]
  }

  DigestCount = ReadBE32 (&Resp[Offset]);
  Offset     += 4;

  if (DigestCount >= 1) {
    UINT16  DigestSize;

    DigestSize = ReadBE16 (&Resp[Offset]);
    Offset    += 2;

    if (DigestSize == 32) {
      CopyMem (Digest, &Resp[Offset], 32);
      *DigestPresent = TRUE;
    }
  }

  return EFI_SUCCESS;
}

VOID
EFIAPI
ShowTpm2Pcrs (
  VOID
  )
{
  EFI_STATUS                            Status;
  EFI_TCG2_PROTOCOL                     *Tcg2;
  EFI_TCG2_BOOT_SERVICE_CAPABILITY      Capability;
  UINT8                                 PcrIndex;

  ClearScreen ();
  PrintTitle (L"TPM2 - LECTURA DE PCRs (SHA-256)");

  Status = gBS->LocateProtocol (&gEfiTcg2ProtocolGuid, NULL, (VOID **)&Tcg2);
  if (EFI_ERROR (Status)) {
    PrintError (L"EFI_TCG2_PROTOCOL no disponible: no hay TPM2 o esta deshabilitado");
    Pause ();
    return;
  }

  ZeroMem (&Capability, sizeof (Capability));
  Capability.Size = sizeof (Capability);

  Status = Tcg2->GetCapability (Tcg2, &Capability);
  if (EFI_ERROR (Status) || !Capability.TPMPresentFlag) {
    PrintError (L"TPM2 no presente segun GetCapability");
    Pause ();
    return;
  }

  PrintSuccess (L"TPM2 detectado, leyendo PCR 0-7 (banco SHA-256)...");
  Print (L"\n");
  PrintInfo (L"Nota: lectura por comando TPM2_PCR_Read crudo, funcionalidad avanzada.");
  Print (L"\n");

  for (PcrIndex = 0; PcrIndex < 8; PcrIndex++) {
    UINT8    Digest[32];
    BOOLEAN  Present;
    UINTN    b;

    Status = Tpm2ReadPcrSha256 (Tcg2, PcrIndex, Digest, &Present);

    Print (L"  PCR[%d]: ", PcrIndex);

    if (EFI_ERROR (Status)) {
      Print (L"error leyendo (%r)\n", Status);
      continue;
    }

    if (!Present) {
      Print (L"sin datos en el banco SHA-256\n");
      continue;
    }

    for (b = 0; b < 32; b++) {
      Print (L"%02x", Digest[b]);
    }
    Print (L"\n");
  }

  Print (L"\n");
  Pause ();
}

/*==========================================================================
  ENTRADA PRINCIPAL
==========================================================================*/

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
  gST->ConOut->EnableCursor (gST->ConOut, FALSE);

  ExitApp = FALSE;

  while (!ExitApp) {
    ShowMainMenu ();
    Choice = GetMenuChoice ();

    switch (Choice) {
      case 1:
        GenerateForensicReport ();
        break;
      case 2:
        CompareAgainstWhitelist ();
        break;
      case 3:
        ShowBootVariables ();
        break;
      case 4:
        ShowSecureBootVariables ();
        break;
      case 5:
        ShowGptPartitions ();
        break;
      case 6:
        ShowTpm2Pcrs ();
        break;
      case 0:
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

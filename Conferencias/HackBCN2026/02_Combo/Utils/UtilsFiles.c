//  _____                                                             _____
// ( ___ )                                                           ( ___ )
//  |   |~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~|   |
//  |   |  _   _  _    _  _       _____  _  _                   ____  |   |
//  |   | | | | || |_ (_)| | ___ |  ___|(_)| |  ___  ___       / ___| |   |
//  |   | | | | || __|| || |/ __|| |_   | || | / _ \/ __|     | |     |   |
//  |   | | |_| || |_ | || |\__ \|  _|  | || ||  __/\__ \  _  | |___  |   |
//  |   |  \___/  \__||_||_||___/|_|    |_||_| \___||___/ (_)  \____| |   |
//  |___|~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~|___|
// (_____)                                                           (_____)

#include "UtilsFiles.h"


// ============================================================================
// STATIC HELPERS
// ============================================================================

STATIC
EFI_STATUS
UtilsFiles_NormalizeShellPath (
    IN  CONST CHAR16  *InputPath,
    OUT CHAR16       **OutputPath
)
{
    UINTN  Index;
    UINTN  PathLen;
    CHAR16 *Path;

    if (InputPath == NULL || OutputPath == NULL)
    {
        return EFI_INVALID_PARAMETER;
    }

    *OutputPath = NULL;
    PathLen = StrLen(InputPath);
    Path = AllocateZeroPool((PathLen + 1) * sizeof(CHAR16));
    if (Path == NULL)
    {
        return EFI_OUT_OF_RESOURCES;
    }

    for (Index = 0; Index < PathLen; Index++)
    {
        Path[Index] = (InputPath[Index] == L'/') ? L'\\' : InputPath[Index];
    }
    Path[PathLen] = L'\0';

    *OutputPath = Path;
    return EFI_SUCCESS;
}


STATIC
EFI_STATUS
UtilsFiles_ExtractEfiPathFromShellPath (
    IN  CONST CHAR16  *ShellPath,
    OUT CHAR16       **EfiPath
)
{
    CONST CHAR16 *PathPart;
    UINTN        PathLen;
    BOOLEAN      NeedsLeadingSlash;
    CHAR16       *Result;
    UINTN        ResultChars;
    UINTN        Index;

    if (ShellPath == NULL || EfiPath == NULL)
    {
        return EFI_INVALID_PARAMETER;
    }

    *EfiPath  = NULL;
    PathPart  = ShellPath;

    for (Index = 0; ShellPath[Index] != L'\0'; Index++)
    {
        if (ShellPath[Index] == L':')
        {
            PathPart = &ShellPath[Index + 1];
            break;
        }
    }

    if (PathPart[0] == L'\0')
    {
        PathPart = L"\\";
    }

    NeedsLeadingSlash = (PathPart[0] != L'\\' && PathPart[0] != L'/');
    PathLen      = StrLen(PathPart);
    ResultChars  = PathLen + (NeedsLeadingSlash ? 1 : 0) + 1;

    Result = AllocateZeroPool(ResultChars * sizeof(CHAR16));
    if (Result == NULL)
    {
        return EFI_OUT_OF_RESOURCES;
    }

    if (NeedsLeadingSlash)
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
        if (Result[Index] == L'/')
        {
            Result[Index] = L'\\';
        }
    }

    *EfiPath = Result;
    return EFI_SUCCESS;
}


STATIC
EFI_STATUS
UtilsFiles_ShellPathExists (
    IN CONST CHAR16 *Path
)
{
    EFI_STATUS        Status;
    SHELL_FILE_HANDLE FileHandle;

    if (Path == NULL)
    {
        return EFI_INVALID_PARAMETER;
    }

    FileHandle = NULL;
    Status = ShellOpenFileByName((CHAR16 *)Path, &FileHandle, EFI_FILE_MODE_READ, 0);

    if (!EFI_ERROR(Status) && FileHandle != NULL)
    {
        ShellCloseFile(&FileHandle);
    }

    return Status;
}


// ============================================================================
// PUBLIC FUNCTIONS
// ============================================================================

VOID
UtilsFiles_TransformBufferXor (
    IN OUT UINT8       *Buffer,
    IN     UINTN       BufferSize,
    IN     CONST UINT8 *XorKey,
    IN     UINTN       XorKeySize
)
{
    UINTN Index;

    if (Buffer == NULL || BufferSize == 0 || XorKey == NULL || XorKeySize == 0)
    {
        return;
    }

    for (Index = 0; Index < BufferSize; Index++)
    {
        Buffer[Index] = Buffer[Index] ^ XorKey[Index % XorKeySize];
    }
}


/**
 * @brief Comprueba si una carpeta existe y se puede abrir en algún volumen UEFI.
 *
 * @details
 * La ruta se puede pasar como ruta de Shell, por ejemplo L"fs2:\\data".
 * Internamente se normaliza a ruta EFI, por ejemplo L"\\data", y se busca
 * en todos los volúmenes que expongan EFI_SIMPLE_FILE_SYSTEM_PROTOCOL.
 */
EFI_STATUS
UtilsFiles_DirectoryExists (
    IN CONST CHAR16 *DirectoryPath
)
{
    EFI_STATUS                      Status;
    EFI_STATUS                      FirstOpenError;
    EFI_HANDLE                      *HandleBuffer;
    UINTN                           HandleCount;
    UINTN                           Index;
    EFI_SIMPLE_FILE_SYSTEM_PROTOCOL *FileSystem;
    EFI_FILE_PROTOCOL               *Root;
    EFI_FILE_PROTOCOL               *Directory;
    CHAR16                          *NormalizedPath;
    CHAR16                          *EfiPath;

    if (DirectoryPath == NULL)
    {
        return EFI_INVALID_PARAMETER;
    }

    HandleBuffer   = NULL;
    HandleCount    = 0;
    FirstOpenError = EFI_NOT_FOUND;
    NormalizedPath = NULL;
    EfiPath        = NULL;

    Status = UtilsFiles_NormalizeShellPath(DirectoryPath, &NormalizedPath);
    if (EFI_ERROR(Status))
    {
        return Status;
    }

    Status = UtilsFiles_ShellPathExists(NormalizedPath);
    if (!EFI_ERROR(Status))
    {
        Print(L"[ SUCCESS ]-{ UtilsFiles.c }-( DirectoryExists )-> Ruta Shell accesible directamente: %s\n", NormalizedPath);
        FreePool(NormalizedPath);
        return EFI_SUCCESS;
    }

    Print(L"[ WARN ]-{ UtilsFiles.c }-( DirectoryExists )-> No se pudo abrir directamente %s: %r. Probando fallback por volumenes...\n", NormalizedPath, Status);

    Status = UtilsFiles_ExtractEfiPathFromShellPath(NormalizedPath, &EfiPath);
    if (EFI_ERROR(Status))
    {
        FreePool(NormalizedPath);
        return Status;
    }

    Print(L"[ INFO ]-{ UtilsFiles.c }-( DirectoryExists )-> Comprobando fallback con ruta EFI interna: %s\n", EfiPath);

    Status = gBS->LocateHandleBuffer(
        ByProtocol,
        &gEfiSimpleFileSystemProtocolGuid,
        NULL,
        &HandleCount,
        &HandleBuffer
    );

    if (EFI_ERROR(Status))
    {
        Print(L"[ WARN ]-{ UtilsFiles.c }-( DirectoryExists )-> No se pudieron localizar volumenes: %r\n", Status);
        goto CleanUp;
    }

    for (Index = 0; Index < HandleCount; Index++)
    {
        FileSystem = NULL;
        Root       = NULL;
        Directory  = NULL;

        Status = gBS->HandleProtocol(
            HandleBuffer[Index],
            &gEfiSimpleFileSystemProtocolGuid,
            (VOID **)&FileSystem
        );

        if (EFI_ERROR(Status))
        {
            continue;
        }

        Status = FileSystem->OpenVolume(FileSystem, &Root);
        if (EFI_ERROR(Status))
        {
            continue;
        }

        Status = Root->Open(
            Root,
            &Directory,
            (CHAR16 *)EfiPath,
            EFI_FILE_MODE_READ,
            0
        );

        if (!EFI_ERROR(Status))
        {
            Print(L"[ SUCCESS ]-{ UtilsFiles.c }-( DirectoryExists )-> Carpeta accesible en volumen #%u: %s\n", Index, EfiPath);
            Directory->Close(Directory);
            Root->Close(Root);
            Status = EFI_SUCCESS;
            goto CleanUp;
        }

        if (FirstOpenError == EFI_NOT_FOUND)
        {
            FirstOpenError = Status;
        }

        if (Root != NULL)
        {
            Root->Close(Root);
        }
    }

    Print(L"[ INFO ]-{ UtilsFiles.c }-( DirectoryExists )-> La carpeta no es accesible todavia. Ultimo error: %r\n", FirstOpenError);
    Status = FirstOpenError;

CleanUp:
    if (HandleBuffer != NULL)  { FreePool(HandleBuffer); }
    if (EfiPath != NULL)       { FreePool(EfiPath); }
    if (NormalizedPath != NULL){ FreePool(NormalizedPath); }

    return Status;
}


EFI_STATUS
UtilsFiles_WriteFile (
    IN EFI_HANDLE   OptionalVolumeHandle,
    IN CONST CHAR16 *FilePath,
    IN CONST VOID   *Data,
    IN UINTN        DataSize
)
{
    Print(L"[ INFO ]-{ UtilsFiles.c }-( WriteFile )-> Inicializando funcion para escribir %d bytes en: %s\n", DataSize, FilePath);

    if (FilePath == NULL || Data == NULL)
    {
        Print(L"[ ERROR ]-{ UtilsFiles.c }-( WriteFile )-> FilePath y Data no pueden ser NULL.\n");
        return EFI_INVALID_PARAMETER;
    }

    EFI_STATUS                  Status;
    EFI_LOADED_IMAGE_PROTOCOL  *ThisLoadedImage = NULL;
    EFI_SIMPLE_FILE_SYSTEM_PROTOCOL *FileSystem = NULL;
    EFI_FILE_PROTOCOL          *Root            = NULL;
    EFI_FILE_PROTOCOL          *File            = NULL;
    UINTN                       WriteSize       = DataSize;
    EFI_HANDLE                  HandleToUse     = OptionalVolumeHandle;

    if (HandleToUse == NULL)
    {
        Print(L"[ INFO ]-{ UtilsFiles.c }-( WriteFile )-> OptionalVolumeHandle es NULL. Determinando volumen actual...\n");

        Status = gBS->HandleProtocol(
            gImageHandle,
            &gEfiLoadedImageProtocolGuid,
            (VOID **)&ThisLoadedImage
        );

        if (EFI_ERROR(Status))
        {
            Print(L"[ ERROR ]-{ UtilsFiles.c }-( WriteFile )-> No se pudo obtener LoadedImageProtocol: %r\n", Status);
            return Status;
        }
        HandleToUse = ThisLoadedImage->DeviceHandle;
    }

    Status = gBS->HandleProtocol(
        HandleToUse,
        &gEfiSimpleFileSystemProtocolGuid,
        (VOID **)&FileSystem
    );

    if (EFI_ERROR(Status))
    {
        Print(L"[ ERROR ]-{ UtilsFiles.c }-( WriteFile )-> No se pudo obtener EfiSimpleFileSystemProtocol: %r\n", Status);
        return Status;
    }

    Status = FileSystem->OpenVolume(FileSystem, &Root);
    if (EFI_ERROR(Status))
    {
        Print(L"[ ERROR ]-{ UtilsFiles.c }-( WriteFile )-> No se pudo abrir el volumen raiz: %r\n", Status);
        goto CleanUp;
    }

    Status = Root->Open(
        Root,
        &File,
        (CHAR16 *)FilePath,
        EFI_FILE_MODE_READ | EFI_FILE_MODE_WRITE | EFI_FILE_MODE_CREATE,
        0
    );

    if (EFI_ERROR(Status))
    {
        Print(L"[ ERROR ]-{ UtilsFiles.c }-( WriteFile )-> No se pudo abrir/crear el fichero destino: %r\n", Status);
        goto CleanUp;
    }

    Status = File->Write(File, &WriteSize, (VOID *)Data);

    if (EFI_ERROR(Status))
    {
        Print(L"[ ERROR ]-{ UtilsFiles.c }-( WriteFile )-> No se pudo escribir en el fichero destino: %r\n", Status);
        goto CleanUp;
    }

    if (WriteSize != DataSize)
    {
        Print(L"[ ERROR ]-{ UtilsFiles.c }-( WriteFile )-> Escritura incompleta en fichero destino.\n");
        Status = EFI_DEVICE_ERROR;
        goto CleanUp;
    }

    Status = File->Flush(File);
    Print(L"[ SUCCESS ]-{ UtilsFiles.c }-( WriteFile )-> Datos escritos correctamente!\n");

CleanUp:
    if (File != NULL) { File->Close(File); }
    if (Root != NULL) { Root->Close(Root); }

    return Status;
}


EFI_STATUS
UtilsFiles_LoadAndStartImageFile (
    IN  CONST CHAR16 *FilePath,
    OUT EFI_HANDLE   *FileImageHandle
)
{
    Print(L"[ INFO ]-{ UtilsFiles.c }-( LoadAndStartImageFile )-> Iniciando carga de driver: %s\n", FilePath);

    if (FilePath == NULL || FileImageHandle == NULL)
    {
        return EFI_INVALID_PARAMETER;
    }

    *FileImageHandle = NULL;

    EFI_STATUS                 Status;
    EFI_LOADED_IMAGE_PROTOCOL *ThisLoadedImage  = NULL;
    EFI_DEVICE_PATH_PROTOCOL  *DriverDevicePath = NULL;
    EFI_HANDLE                 LocalDriverHandle = NULL;
    UINTN                      HandleCount;
    EFI_HANDLE                *HandleBuffer;

    Status = gBS->HandleProtocol(
        gImageHandle,
        &gEfiLoadedImageProtocolGuid,
        (VOID **)&ThisLoadedImage
    );

    if (EFI_ERROR(Status))
    {
        Print(L"[ ERROR ]-{ UtilsFiles.c }-( LoadAndStartImageFile )-> No se pudo obtener LoadedImageProtocol: %r\n", Status);
        return Status;
    }

    DriverDevicePath = FileDevicePath(ThisLoadedImage->DeviceHandle, FilePath);

    if (DriverDevicePath == NULL)
    {
        Print(L"[ ERROR ]-{ UtilsFiles.c }-( LoadAndStartImageFile )-> No se pudo crear el Device Path.\n");
        return EFI_OUT_OF_RESOURCES;
    }

    Status = gBS->LoadImage(
        FALSE,
        gImageHandle,
        DriverDevicePath,
        NULL,
        0,
        &LocalDriverHandle
    );

    FreePool(DriverDevicePath);

    if (EFI_ERROR(Status))
    {
        Print(L"[ ERROR ]-{ UtilsFiles.c }-( LoadAndStartImageFile )-> gBS->LoadImage fallo: %r\n", Status);
        return Status;
    }

    Print(L"[ SUCCESS ]-{ UtilsFiles.c }-( LoadAndStartImageFile )-> Driver cargado. Handle: 0x%p\n", LocalDriverHandle);

    Status = gBS->StartImage(LocalDriverHandle, NULL, NULL);

    if (EFI_ERROR(Status))
    {
        Print(L"[ ERROR ]-{ UtilsFiles.c }-( LoadAndStartImageFile )-> gBS->StartImage fallo: %r\n", Status);
        gBS->UnloadImage(LocalDriverHandle);
        return Status;
    }

    Print(L"[ SUCCESS ]-{ UtilsFiles.c }-( LoadAndStartImageFile )-> Driver iniciado correctamente!\n");

    Status = gBS->LocateHandleBuffer(AllHandles, NULL, NULL, &HandleCount, &HandleBuffer);

    if (!EFI_ERROR(Status))
    {
        for (UINTN Index = 0; Index < HandleCount; Index++)
        {
            gBS->ConnectController(HandleBuffer[Index], NULL, NULL, TRUE);
        }
        FreePool(HandleBuffer);
        Print(L"[ SUCCESS ]-{ UtilsFiles.c }-( LoadAndStartImageFile )-> Ciclo de conexion finalizado!\n");
    }
    else
    {
        Print(L"[ ERROR ]-{ UtilsFiles.c }-( LoadAndStartImageFile )-> No se pudo obtener lista de handles: %r\n", Status);
    }

    *FileImageHandle = LocalDriverHandle;
    Print(L"[ SUCCESS ]-{ UtilsFiles.c }-( LoadAndStartImageFile )-> Proceso completo. Handle: 0x%p\n", *FileImageHandle);

    return EFI_SUCCESS;
}

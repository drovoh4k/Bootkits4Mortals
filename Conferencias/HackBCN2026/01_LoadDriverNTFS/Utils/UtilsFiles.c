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

// ============================================================================
// INCLUDES
// ============================================================================

#include "UtilsFiles.h"

// ============================================================================
// FUNCTIONS
// ============================================================================

EFI_STATUS
UtilsFiles_WriteFile(
    IN EFI_HANDLE OptionalVolumeHandle,
    IN CONST CHAR16 *FilePath,
    IN CONST VOID *Data,
    IN UINTN DataSize
)
{
    //
    // --- SECCIÓN 1: PREPARACIÓN Y VALIDACIÓN ---
    //

    Print(L"[ INFO ]-{ UtilsFiles.c }-( WriteFile )-> Incializando funcion para escribir %d bytes en: %s\n", DataSize, &FilePath);

    if (FilePath == NULL || Data == NULL)
    {
        Print(L"[ ERROR ]-{ UtilsFiles.c }-( WriteFile )-> Los argumentos FilePath y Data no pueden ser NULL.\n");
        return EFI_INVALID_PARAMETER;
    }

    EFI_STATUS Status;
    EFI_LOADED_IMAGE_PROTOCOL *ThisLoadedImage = NULL;
    EFI_SIMPLE_FILE_SYSTEM_PROTOCOL *FileSystem = NULL;
    EFI_FILE_PROTOCOL *Root = NULL;
    EFI_FILE_PROTOCOL *File = NULL;
    UINTN WriteSize = DataSize;
    EFI_HANDLE HandleToUse = OptionalVolumeHandle;

    //
    // --- SECCIÓN 2: DETERMINAR VOLUMEN DE DESTINO ---
    //

    // En caso que no se haya pasado un handle de volumen especifico,
    // determinamos el volumen desde el que se ejecuta esta aplicación
    if (HandleToUse == NULL)
    {
        Print(L"[ INFO ]-{ UtilsFiles.c }-( WriteFile )-> OptionalVolumeHandle es NULL. Determinando volumen actual...");

        // Obtenemos el protocolo de nuestra propia aplicación para saber en qué
        // dispositivo (partición) nos estamos ejecutando
        Status = gBS->HandleProtocol(
            gImageHandle,
            &gEfiLoadedImageProtocolGuid,
            (VOID **)&ThisLoadedImage
        );
        
        if (EFI_ERROR(Status))
        {
            Print(L"[ ERROR ]-{ UtilsFiles.c }-( WriteFile )-> No se pudo obtener LoadedImageProtocol para determinar el volumen actual: %r\n", Status);
            return Status;
        }
        HandleToUse = ThisLoadedImage->DeviceHandle;
    }

    //
    // --- SECCIÓN 3: ESCRIBIR EL FICHERO DESTINO ---
    //

    // Paso 3.1: Obtener un puntero a la interfaz del protocolo para interactuar con el volumen válido

    Print(L"[ INFO ]-{ UtilsFiles.c }-( WriteFile )-> Obteniendo puntero a la interfaz para interactuar con el volumen valido...\n");

    Status = gBS->HandleProtocol(
        HandleToUse,
        &gEfiSimpleFileSystemProtocolGuid,
        (VOID **)&FileSystem
    );

    if (EFI_ERROR(Status))
    {
        Print(L"[ ERROR ]-{ UtilsFiles.c }-( WriteFile )-> No se pudo obtener EfiSimpleFileSystemProtocol a partir del handle: %r\n", Status);
        return Status;
    }

    // Paso 3.2: Abrir el volumen para obtener un handle al directorio raíz ('\').

    Print(L"[ INFO ]-{ UtilsFiles.c }-( WriteFile )-> Abriendo volumen para obtener un handle del directorio raiz...\n");

    Status = FileSystem->OpenVolume(
        FileSystem,
        &Root
    );

    if (EFI_ERROR(Status))
    {
        Print(L"[ ERROR ]-{ UtilsFiles.c }-( WriteFile )-> No se pudo obtener el directorio raiz a partir del puntero FileSystem: %r\n", Status);
        goto CleanUp;
    }

    // Paso 3.3: Abrir (o crear) el fichero de destino

    Print(L"[ INFO ]-{ UtilsFiles.c }-( WriteFile )-> Abriendo (o creando) el fichero de destino...\n");

    Status = Root->Open(
        Root,
        &File,
        (CHAR16 *)FilePath,
        EFI_FILE_MODE_READ | EFI_FILE_MODE_WRITE | EFI_FILE_MODE_CREATE,
        0
    );

    if (EFI_ERROR(Status))
    {
        Print(L"[ ERROR ]-{ UtilsFiles.c }-( WriteFile )-> No se pudo abrir (o crear) el fichero de destino: %r\n", Status); 
        goto CleanUp;
    }

    // Paso 3.4: Escribir los datos en el fichero

    Print(L"[ INFO ]-{ UtilsFiles.c }-( WriteFile )-> Escribiendo los datos en el fichero de destino...\n");

    Status = File->Write(File, &WriteSize, (VOID *)Data);
    
    if (EFI_ERROR(Status))
    {
        Print(L"[ ERROR ]-{ UtilsFiles.c }-( WriteFile )-> No se pudo escribir en fichero de destino: %r\n", Status); 
        goto CleanUp;
    }
    if (WriteSize != DataSize)
    {
        Print(L"[ ERROR ]-{ UtilsFiles.c }-( WriteFile )-> No se pudieron escribir todos los datos en fichero de destino: %r\n", Status); 
        Status = EFI_DEVICE_ERROR;
        goto CleanUp;
    }

    // Paso 3.5: Volcar los buffers a disco para asegurar la persistencia

    Print(L"[ INFO ]-{ UtilsFiles.c }-( WriteFile )-> Haciendo flush del fichero para assegurar la persistencia...\n");

    Status = File->Flush(File);

    Print(L"[ SUCCESS ]-{ UtilsFiles.c }-( WriteFile )-> Se han escrito todos los datos correctamente!\n");

    //
    // --- SECCIÓN 4: LIMPIEZA DE RECURSOS ---
    //
    
    CleanUp:
        Print(L"[ INFO ]-{ UtilsFiles.c }-( WriteFile )-> Haciendo clean up...\n");

        // El patrón 'goto CleanUp' garantiza que siempre liberemos los recursos
        // que hemos abierto, sin importar en qué punto del proceso ocurra un error
        if (File != NULL)
        {
            File->Close(File);
        }
        if (Root != NULL)
        {
            Root->Close(Root);
        }

    Print(L"[ INFO ]-{ UtilsFiles.c }-( LoadAndStartImageFile )-> Saliendo de la funcion\n");

    return Status;
}


EFI_STATUS
UtilsFiles_LoadAndStartImageFile(
    IN CONST CHAR16 *FilePath,
    OUT EFI_HANDLE *FileImageHandle
)
{
    //
    // --- SECCIÓN 1: PREPARACIÓN Y VALIDACIÓN ---
    //

    Print(L"[ INFO ]-{ UtilsFiles.c }-( LoadAndStartImageFile )-> Iniciando proceso de carga de driver para: %s \n", FilePath);

    if (FilePath == NULL || FileImageHandle == NULL)
    {
        return EFI_INVALID_PARAMETER;
    }

    // Inicializamos el handle de salida a NULL para que solo se rellene si todo el
    // proceso tiene éxito
    *FileImageHandle = NULL;

    EFI_STATUS Status;
    EFI_LOADED_IMAGE_PROTOCOL *ThisLoadedImage = NULL;
    EFI_DEVICE_PATH_PROTOCOL *DriverDevicePath = NULL;
    EFI_HANDLE LocalDriverHandle = NULL;
    UINTN HandleCount;
    EFI_HANDLE *HandleBuffer;

    //
    // --- SECCIÓN 2: LOCALIZAR Y CARGAR EL DRIVER (LOAD) ---
    //

    // Paso 2.1: Obtener el handle de nuestra aplicación

    Print(L"[ INFO ]-{ UtilsFiles.c }-( LoadAndStartImageFile )-> Obteniendo el handle de nuestra aplicacion...\n");
    
    Status = gBS->HandleProtocol(
        gImageHandle,
        &gEfiLoadedImageProtocolGuid,
        (VOID **)&ThisLoadedImage
    );
    
        if (EFI_ERROR(Status))
    {
        Print(L"[ ERROR ]-{ UtilsFiles.c }-( LoadAndStartImageFile )-> No se pudo obtener el protocolo LoadedImage: %r\n", Status);
        return Status;
    }

    // Paso 2.2: Construir el Device Path para el driver
    
    Print(L"[ INFO ]-{ UtilsFiles.c }-( LoadAndStartImageFile )-> Construyendo el Device Path para el driver\n");
    
    DriverDevicePath = FileDevicePath(
        ThisLoadedImage->DeviceHandle, 
        FilePath
    );
    
    if (DriverDevicePath == NULL)
    {
        Print(L"[ ERROR ]-{ UtilsFiles.c }-( LoadAndStartImageFile )-> No se pudo crear el Device Path\n");
        return EFI_OUT_OF_RESOURCES;
    }

    // Paso 2.3: Cargar la imagen del driver en memoria (LoadImage)
    
    Print(L"[ INFO ]-{ UtilsFiles.c }-( LoadAndStartImageFile )-> Cargando la imagen del driver en memoria (LoadImage)...\n");
    
    Status = gBS->LoadImage(
        FALSE,
        gImageHandle,
        DriverDevicePath,
        NULL,
        0,
        &LocalDriverHandle
    );

    // Liberamos el Device Path en cuanto deja de ser necesario
    FreePool(DriverDevicePath); 

    if (EFI_ERROR(Status))
    {
        Print(L"[ ERROR ]-{ UtilsFiles.c }-( LoadAndStartImageFile )-> gBS->LoadImage fallo\n", Status);
        return Status;
    }
    
    Print(L"[ SUCCESS ]-{ UtilsFiles.c }-( LoadAndStartImageFile )-> Driver cargado! Handle asignado: 0x%p\n", LocalDriverHandle);

    //
    // --- SECCIÓN 3: INICIAR EL DRIVER (START) ---
    //

    Print(L"[ INFO ]-{ UtilsFiles.c }-( LoadAndStartImageFile )-> Iniciando el driver (StartImage)...\n");
    
    Status = gBS->StartImage(
        LocalDriverHandle,
        NULL,
        NULL);
    
    if (EFI_ERROR(Status))
    {
        Print(L"[ ERROR ]-{ UtilsFiles.c }-( LoadAndStartImageFile )-> gBS->StartImage fallo: %r\n", Status);

        // Si falla el inicio, descargamos la imagen
        gBS->UnloadImage(LocalDriverHandle);

        return Status;
    }
    
    Print(L"[ SUCCESS ]-{ UtilsFiles.c }-( LoadAndStartImageFile )-> El driver se ha iniciado correctamente!\n");

    //
    // --- SECCIÓN 4: CONECTAR DRIVER ---
    //

    Print(L"[ INFO ]-{ UtilsFiles.c }-( LoadAndStartImageFile )-> Lanzando ciclo de conexion general (ConnectController)...\n");

    // Paso 4.1: Obtener lista de todos los handles

    Status = gBS->LocateHandleBuffer(
        AllHandles,
        NULL,
        NULL,
        &HandleCount,
        &HandleBuffer
    );
    
    // Paso 4.2: Lanzar el ciclo de conexión

    if (!EFI_ERROR(Status))
    {
        for (UINTN Index = 0; Index < HandleCount; Index++)
        {
            gBS->ConnectController(
                HandleBuffer[Index],
                NULL,
                NULL,
                TRUE
            );
        }
        FreePool(HandleBuffer);
        Print(L"[ SUCCESS ]-{ UtilsFiles.c }-( LoadAndStartImageFile )-> Ciclo de conexion finalizado!\n");
    }
    else
    {
        Print(L"[ ERROR ]-{ UtilsFiles.c }-( LoadAndStartImageFile )-> No se pudo obtener la lista de handles para la conexion: %r\n", Status);
    }

    //
    // --- SECCIÓN 5: FINALIZACIÓN Y DEVOLUCIÓN DEL HANDLE ---
    //
    
    // Si hemos llegado hasta aquí, todo el proceso ha sido un éxito
    // Asignamos el handle local al puntero de salida que nos ha dado el caller
    *FileImageHandle = LocalDriverHandle;

    Print(L"[ SUCCESS ]-{ UtilsFiles.c }-( LoadAndStartImageFile )-> Proceso finalizado con exito! Devolviendo handle 0x%p\n", *FileImageHandle);
    
    return EFI_SUCCESS;
}
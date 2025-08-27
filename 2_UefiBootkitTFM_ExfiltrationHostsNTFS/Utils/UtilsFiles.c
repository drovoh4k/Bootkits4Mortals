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
UtilsFiles_ReadFile(
    IN  EFI_HANDLE   VolumeHandle,
    IN  CONST CHAR16 *FilePath,
    OUT VOID         **Data,
    OUT UINTN        *DataSize
)
{
    //
    // --- SECCIÓN 1: PREPARACIÓN Y VALIDACIÓN ---
    //

    Print(L"[ INFO ]-{ UtilsFiles.c }-( ReadFile )-> Intentando leer el fichero: %s\n", FilePath);

    if (VolumeHandle == NULL || FilePath == NULL || Data == NULL || DataSize == NULL) {
        return EFI_INVALID_PARAMETER;
    }
    *Data = NULL;
    *DataSize = 0;

    EFI_STATUS                        Status;
    EFI_SIMPLE_FILE_SYSTEM_PROTOCOL   *FileSystem = NULL;
    EFI_FILE_PROTOCOL                 *Root = NULL;
    EFI_FILE_PROTOCOL                 *File = NULL;
    EFI_FILE_INFO                     *FileInfo = NULL;
    UINTN                             FileInfoSize = 0;

    //
    // --- SECCIÓN 2: ABRIR EL FICHERO ---
    //

    // Paso 2.1: Obtener un puntero a la interfaz del protocolo para interactuar con el volumen válido

    Print(L"[ INFO ]-{ UtilsFiles.c }-( ReadFile )-> Obteniendo puntero a la interfaz para interactuar con el volumen valido...\n");

    Status = gBS->HandleProtocol(
        VolumeHandle,
        &gEfiSimpleFileSystemProtocolGuid,
        (VOID**)&FileSystem
    );

    if (EFI_ERROR(Status))
    {
        Print(L"[ ERROR ]-{ UtilsFiles.c }-( ReadFile )-> No se pudo obtener EfiSimpleFileSystemProtocol a partir del handle: %r\n", Status);
        return Status;
    }

    // Paso 2.2: Abrir el volumen para obtener un handle al directorio raíz ('\').

    Print(L"[ INFO ]-{ UtilsFiles.c }-( Readfile )-> Abriendo volumen para obtener un handle del directorio raiz...\n");

    Status = FileSystem->OpenVolume(FileSystem, &Root);
    
    if (EFI_ERROR(Status))
    {
        Print(L"[ ERROR ]-{ UtilsFiles.c }-( ReadFile )-> No se pudo obtener el directorio raiz a partir del puntero FileSystem: %r\n", Status);
        goto CleanUp;
    }

    // Paso 2.3: Abrir (o crear) el fichero de destino

    Print(L"[ INFO ]-{ UtilsFiles.c }-( ReadFile )-> Abriendo (o creando) el fichero de destino...\n");

    Status = Root->Open(
        Root,
        &File,
        (CHAR16*)FilePath,
        EFI_FILE_MODE_READ,
        0
    );

    if (EFI_ERROR(Status)) {
        Print(L"[ ERROR ]-{ UtilsFiles.c }-( ReadFile )-> No se pudo abrir (o crear) el fichero de destino: %r\n", Status); 
        goto CleanUp;
    }

    //
    // --- SECCIÓN 3: OBTENER TAMAÑO DEL FICHERO ---
    //

    // Paso 3.1: Consultar el tamaño de la información del fichero

    Print(L"[ INFO ]-{ UtilsFiles.c }-( ReadFile )-> Consultando tamaño de la info del fichero...\n");
    
    Status = File->GetInfo(
        File,
        &gEfiFileInfoGuid, 
        &FileInfoSize,
        NULL
    );
    
    if (Status != EFI_BUFFER_TOO_SMALL)
    {
        Print(L"[ ERROR ]-{ UtilsFiles.c }-( ReadFile )-> Fallo al obtener el tamaño de la info del fichero: %r\n", Status);
        goto CleanUp;
    }

    // Paso 3.2: Reservar memoria para la información del fichero

    Print(L"[ INFO ]-{ UtilsFiles.c }-( ReadFile )-> Reservando memoria para la info del fichero...\n");

    FileInfo = AllocatePool(FileInfoSize);

    if (FileInfo == NULL)
    {
        Print(L"[ ERROR ]-{ UtilsFiles.c }-( ReadFile )-> No se pudo reservar memoria para FileInfo\n");
        Status = EFI_OUT_OF_RESOURCES;
        goto CleanUp;
    }

    // Paso 3.3: Obtener la información del fichero

    Print(L"[ INFO ]-{ UtilsFiles.c }-( ReadFile )-> Paso 3.3: Obteniendo la info del fichero...\n");

    Status = File->GetInfo(
        File,
        &gEfiFileInfoGuid,
        &FileInfoSize,
        FileInfo
    );

    if (EFI_ERROR(Status))
    {
        Print(L"[ ERROR ]-{ UtilsFiles.c }-( ReadFile )-> Fallo al obtener la info del fichero: %r\n", Status);
        goto CleanUp;
    }

    //
    // --- SECCIÓN 4: LEER EL FICHERO ---
    //

    // Paso 4.1: Reservar memoria para el contenido del fichero

    Print(L"[ INFO ]-{ UtilsFiles.c }-( ReadFile )-> Reservando memoria para el contenido del fichero...\n");

    // Asignamos el tamaño del fichero a la dirección que apunta el argumento DataSize
    *DataSize = FileInfo->FileSize;

    // Assignamos la dirección de la memoria reservada al argumento Data
    *Data = AllocatePool(*DataSize);

    if (*Data == NULL)
    {
        Print(L"[ ERROR ]-{ UtilsFiles.c }-( ReadFile )-> No se pudo reservar memoria para el contenido del fichero\n");
        Status = EFI_OUT_OF_RESOURCES;
        goto CleanUp;
    }

    // Paso 4.2: Lectura del fichero

    Status = File->Read(
        File,
        DataSize,
        *Data
    );

    if (EFI_ERROR(Status) || *DataSize != FileInfo->FileSize)
    {
        Print(L"[ ERROR ]-{ UtilsFiles.c }-( ReadFile )-> Fallo al leer el fichero o tamaño incorrecto. Status: %r\n", Status);

        // Si falla la lectura, liberamos el la memoria que habíamos reservado.
        FreePool(*Data);
        *Data = NULL;
        *DataSize = 0;

        goto CleanUp;
    }

    Print(L"[ SUCCESS ]-{ UtilsFiles.c }-( ReadFile )-> Fichero leido y cargado en memoria correctamente\n");

    //
    // --- SECCIÓN 5: LIMPIEZA DE RECURSOS ---
    //

    CleanUp:
        // Liberamos todos los recursos, excepto el buffer principal que devolvemos.
        if (FileInfo != NULL)
        {
            FreePool(FileInfo);
        }
        if (File != NULL)
        {
            File->Close(File);
        }
        if (Root != NULL)
        {
            Root->Close(Root);
        }

        return Status;
}


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


/**
 * @brief Encapsula todo el proceso de escritura y carga del driver NTFS.
 *
 * @details
 * Esta función realiza la secuencia completa para activar el driver NTFS:
 * 1. Escribe el fichero del driver en la partición EFI por defecto.
 * 2. Carga, inicia y conecta el driver para que el sistema pueda usarlo.
 *
 * @param[out] DriverHandleOut   Puntero para devolver el handle del driver si
 * la operación tiene éxito.
 *
 * @retval EFI_SUCCESS           El driver se inicializó correctamente.
 * @retval Otros                 Ocurrió un error en alguna de las fases.
 */
EFI_STATUS
UtilsFiles_InitializeNtfsDriver(
	OUT EFI_HANDLE *DriverHandleOut
)
{
	EFI_STATUS Status;
	EFI_HANDLE LocalDriverHandle = NULL;
	CONST CHAR16 *DriverPath = L"\\EFI\\Boot\\ntfs_x64_rw.efi";

	if (DriverHandleOut == NULL)
	{
		return EFI_INVALID_PARAMETER;
	}
	*DriverHandleOut = NULL;

    Print(L"[ INFO ]-{ UtilsFiles.c }-( InitializeNtfsDriver )-> Inicializando driver NTFS\n");

	//
	// --- PASO 1: Escribir el driver NTFS en el disco ---
	//
	
    Print(L"[ INFO ]-{ UtilsFiles.c }-( InitializeNtfsDriver )-> Intentando escribir el driver NTFS en el disco en %s", DriverPath);

	// Llamamos a la función de escritura, pasando NULL como primer argumento
	// para que escriba en la partición por defecto.
	Status = UtilsFiles_WriteFile(
		NULL,
		DriverPath,
		Global_PayloadsNtfsDriver_Ntfs3g,
		sizeof(Global_PayloadsNtfsDriver_Ntfs3g));

	if (EFI_ERROR(Status))
	{
		Print(L"[ ERROR ]-{ UtilsFiles.c }-( InitializeNtfsDriver )-> No se pudo escribir el fichero del driver. Codigo de error: %r\n", Status);
		return Status;
	}
	Print(L"[ SUCCESS ]-{ UtilsFiles.c }-( InitializeNtfsDriver )-> El fichero del driver se ha escrito correctamente en el disco\n");

	//
	// --- PASO 2: Cargar y activar el driver ---
	//
	
    Print(L"[ INFO ]-{ UtilsFiles.c }-( InitializeNtfsDriver )-> Procediendo a cargar, iniciar y conectar el driver\n");

	Status = UtilsFiles_LoadAndStartImageFile(
		DriverPath,
		&LocalDriverHandle);

	if (EFI_ERROR(Status))
	{
		Print(L"[ ERROR ]-{ UtilsFiles.c }-( InitializeNtfsDriver )-> Fallo durante la activacion del driver. Codigo de error: %r\n", Status);
		return Status;
	}

	// Devolvemos el handle.
	*DriverHandleOut = LocalDriverHandle;

	return EFI_SUCCESS;
}


/**
 * @brief Busca la partición del sistema Windows y devuelve su HANDLE de volumen.
 *
 * @details
 * Esta función escanea todos los volúmenes para encontrar la partición de Windows
 * buscando el fichero `\Windows\System32\ntoskrnl.exe`.
 *
 * A diferencia de la versión anterior, esta función devuelve el `EFI_HANDLE` del
 * volumen (la "dirección de la casa"), que es el tipo correcto para pasar a
 * otras funciones como `UtilsFiles_WriteFile`.
 *
 * @param[out] VolumeHandle      Puntero que recibirá el HANDLE del volumen de Windows.
 *
 * @retval EFI_SUCCESS           Se encontró la partición de Windows y se devolvió el handle.
 * @retval EFI_NOT_FOUND         No se encontró ninguna partición de Windows.
 */
EFI_STATUS
UtilsFiles_FindWindowsPartition (
    OUT EFI_HANDLE *VolumeHandle
)
{
    //
    // --- SECCIÓN 1: PREPARACIÓN Y VALIDACIÓN ---
    //
    
    if (VolumeHandle == NULL) {
        return EFI_INVALID_PARAMETER;
    }
    *VolumeHandle = NULL;

    EFI_STATUS Status;
    UINTN HandleCount = 0;
    EFI_HANDLE *HandleBuffer = NULL;
    EFI_STATUS FoundStatus = EFI_NOT_FOUND;

    //
    // --- SECCIÓN 2: BÚSQUEDA DE VOLÚMENES ---
    //

    Print(L"[ INFO ]-{ UtilsFiles.c }-( FindWindowsPartition )-> Buscando la particion del sistema Windows\n");

    Status = gBS->LocateHandleBuffer(
        ByProtocol,
        &gEfiSimpleFileSystemProtocolGuid,
        NULL,
        &HandleCount,
        &HandleBuffer
    );

    if (EFI_ERROR(Status) || HandleCount == 0)
    {
        Print(L"[ ERROR ]-{ UtilsFiles.c }-( FindWindowsPartition )-> No se encontraron volumenes con sistema de ficheros\n");
        return EFI_NOT_FOUND;
    }

    //
    // --- SECCIÓN 3: ANÁLISIS DE CADA VOLUMEN ---
    //

    Print(L"[ INFO ]-{ UtilsFiles.c }-( FindWindowsPartition )-> Se encontraron %d volumenes. Analizando cada uno\n", HandleCount);

    for (UINTN i = 0; i < HandleCount; i++)
    {
        // Paso 3.1: Declarar variables

        EFI_SIMPLE_FILE_SYSTEM_PROTOCOL *FileSystem = NULL;
        EFI_FILE_PROTOCOL *TempRoot = NULL;
        EFI_FILE_PROTOCOL *TestFile = NULL;

        // Paso 3.2: Abrir volumen

        Status = gBS->HandleProtocol(
            HandleBuffer[i],
            &gEfiSimpleFileSystemProtocolGuid,
            (VOID**)&FileSystem
        );
        
        if (EFI_ERROR(Status))
        {
            continue;
        }

        Status = FileSystem->OpenVolume(
            FileSystem,
            &TempRoot
        );

        if (EFI_ERROR(Status))
        {
            continue;
        }

        // Paso 3.3: Abrir el fichero de ntoskrnl.exe

        Status = TempRoot->Open(
            TempRoot,
            &TestFile,
            L"\\Windows\\System32\\ntoskrnl.exe",
            EFI_FILE_MODE_READ,
            0
        );

        // Paso 3.4: Comprobar el resultado

        if (!EFI_ERROR(Status))
        {
            Print(L"[ SUCCESS ]-{ UtilsFiles.c }-( FindWindowsPartition )-> Particion de Windows encontrada en el handle 0x%p\n", HandleBuffer[i]);

            // Devolvemos el handle del volumen (HandleBuffer[i])
            *VolumeHandle = HandleBuffer[i];
            FoundStatus = EFI_SUCCESS;

            // Cerramos los handles del fichero y del directorio
            TestFile->Close(TestFile);
            TempRoot->Close(TempRoot);
            break;
        }

        TempRoot->Close(TempRoot);
    }

    //
    // --- SECCIÓN 4: FINALIZACIÓN Y RETORNO ---
    //

    FreePool(HandleBuffer);
    return FoundStatus;
}
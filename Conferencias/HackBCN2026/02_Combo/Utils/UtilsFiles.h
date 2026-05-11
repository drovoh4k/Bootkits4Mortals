//  _____                                                             _____
// ( ___ )                                                           ( ___ )
//  |   |~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~|   |
//  |   |  _   _  _    _  _       _____  _  _                  _   _  |   |
//  |   | | | | || |_ (_)| | ___ |  ___|(_)| |  ___  ___      | | | | |   |
//  |   | | | | || __|| || |/ __|| |_   | || | / _ \/ __|     | |_| | |   |
//  |   | | |_| || |_ | || |\__ \|  _|  | || ||  __/\__ \  _  |  _  | |   |
//  |   |  \___/  \__||_||_||___/|_|    |_||_| \___||___/ (_) |_| |_| |   |
//  |___|~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~|___|
// (_____)                                                           (_____)


#ifndef _UTILS_FILES_H_
#define _UTILS_FILES_H_


// ============================================================================
// INCLUDES
// ============================================================================

#include <Uefi.h>
#include <Library/UefiLib.h>
#include <Library/BaseLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/DevicePathLib.h>
#include <Library/ShellLib.h>
#include <Guid/FileInfo.h>


// ============================================================================
// FUNCTIONS
// ============================================================================

/**
 * @brief Escribe datos en un fichero en la partición actual o en una específica.
 *
 * @param[in] OptionalVolumeHandle  Handle del volumen destino. NULL = partición actual.
 * @param[in] FilePath              Ruta del fichero. Ej: L"\\EFI\\Boot\\ntfs_x64_rw.efi"
 * @param[in] Data                  Datos a escribir.
 * @param[in] DataSize              Tamaño en bytes.
 */
EFI_STATUS
UtilsFiles_WriteFile (
    IN EFI_HANDLE   OptionalVolumeHandle,
    IN CONST CHAR16 *FilePath,
    IN CONST VOID   *Data,
    IN UINTN        DataSize
);


/**
 * @brief Carga, inicia y conecta un driver EFI. Devuelve su handle.
 *
 * @param[in]  FilePath         Ruta del driver. Ej: L"\\EFI\\Boot\\ntfs_x64_rw.efi"
 * @param[out] FileImageHandle  Recibe el handle del driver si tiene éxito.
 */
EFI_STATUS
UtilsFiles_LoadAndStartImageFile (
    IN  CONST CHAR16 *FilePath,
    OUT EFI_HANDLE   *FileImageHandle
);


/**
 * @brief Comprueba si una carpeta existe y es accesible en algún volumen UEFI.
 *
 * @details
 * Primero intenta la ruta Shell exacta (ej: L"fs2:\\data"). Si falla,
 * extrae la ruta EFI y escanea todos los volúmenes disponibles.
 *
 * @param[in] DirectoryPath  Ruta Shell de la carpeta. Ej: L"fs2:\\data"
 */
EFI_STATUS
UtilsFiles_DirectoryExists (
    IN CONST CHAR16 *DirectoryPath
);


/**
 * @brief Aplica XOR in-place a un buffer con la clave dada.
 *
 * @param[in,out] Buffer      Buffer a transformar.
 * @param[in]     BufferSize  Tamaño en bytes.
 * @param[in]     XorKey      Clave XOR.
 * @param[in]     XorKeySize  Tamaño de la clave en bytes.
 */
VOID
UtilsFiles_TransformBufferXor (
    IN OUT UINT8       *Buffer,
    IN     UINTN       BufferSize,
    IN     CONST UINT8 *XorKey,
    IN     UINTN       XorKeySize
);


#endif

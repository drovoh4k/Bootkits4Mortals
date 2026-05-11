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

// Fichero de inclusión raíz para cualquier módulo UEFI. Define todos los tipos de datos,
// constantes y estructuras estándar del entorno UEFI, permitiendo la portabilidad
// https://github.com/tianocore/edk2/blob/master/MdePkg/Include/Uefi.h
#include <Uefi.h>

// Contiene funciones de alto nivel que simplifican operaciones comunes en UEFI,
// como la gestión de protocolos y la impresión en la consola
// https://github.com/tianocore/edk2/blob/master/MdePkg/Include/Library/UefiLib.h
#include <Library/UefiLib.h>

// Proporciona un conjunto de funciones de utilidad de bajo nivel, como manipulación
// de strings, manejo de listas enlazadas y operaciones matemáticas
// https://github.com/tianocore/edk2/blob/master/MdePkg/Include/Library/BaseLib.h
#include <Library/BaseLib.h>

// Proporciona un servicio para obtener un puntero a la Tabla de Servicios de Arranque (gBS),
// esencial para operaciones disponibles durante la fase de arranque
// https://github.com/tianocore/edk2/blob/master/MdePkg/Include/Library/UefiBootServicesTableLib.h
#include <Library/UefiBootServicesTableLib.h>

// Ofrece servicios para solicitar (AllocatePool) y liberar (FreePool) bloques de memoria.
// Es la librería estándar para la gestión de memoria dinámica en UEFI
// https://github.com/tianocore/edk2/blob/master/MdePkg/Include/Library/MemoryAllocationLib.h
#include <Library/MemoryAllocationLib.h>

// Ofrece funciones para construir y analizar estructuras EFI_DEVICE_PATH_PROTOCOL,
// que identifican de manera única cualquier recurso del sistema
// https://github.com/tianocore/edk2/blob/master/MdePkg/Include/Library/DevicePathLib.h
#include <Library/DevicePathLib.h>

// Proporciona una interfaz para acceder a las funcionalidades del Shell de UEFI,
// útil para crear comandos de Shell o interactuar con su entorno
// https://github.com/tianocore/edk2/blob/master/ShellPkg/Include/Library/ShellLib.h
#include <Library/ShellLib.h>



// ============================================================================
// FUNCTIONS
// ============================================================================

/**
 * @brief Escribe datos en un fichero, permitiendo especificar el volumen de destino
 *
 * @details
 * Puede escribir tanto en la partición actual como en cualquier otra partición
 * especificada.
 *
 * - Si `OptionalVolumeHandle` es `NULL`, la función escribirá en la partición
 * desde la que se ejecuta la aplicación (comportamiento por defecto)
 * 
 * - Si se proporciona un `Handle` en `OptionalVolumeHandle`, la función
 * escribirá el fichero en ese volumen específico
 *
 * @param[in] OptionalVolumeHandle  (Opcional) Handle del volumen de destino. Si es NULL,
 * se usa el volumen de la aplicación actual
 * @param[in] FilePath              Ruta del fichero a escribir. Ejemplo: L"\\mi_fichero.txt"
 * @param[in] Data                  Puntero a los datos que se van a escribir
 * @param[in] DataSize              Tamaño, en bytes, de los datos
 *
 * @retval EFI_SUCCESS               Los datos se escribieron correctamente
 * @retval EFI_INVALID_PARAMETER     Uno de los parámetros de entrada es nulo
 * @retval Otros                     Cualquier otro código de error del sistema
 */
EFI_STATUS
UtilsFiles_WriteFile (
	IN EFI_HANDLE   OptionalVolumeHandle,
	IN CONST CHAR16 *FilePath,
	IN CONST VOID   *Data,
	IN UINTN        DataSize
);


/**
 * @brief Carga, inicia y conecta un driver EFI, y devuelve su handle.
 *
 * @details
 * Realiza los tres pasos fundamentales para activar un driver y
 * devuelve su handle para un posible uso posterior
 *
 * 1.  **Cargar (Load):** Lee el fichero del driver y lo carga en memoria
 * 2.  **Iniciar (Start):** Ejecuta el punto de entrada del driver para que se anuncie
 * 3.  **Conectar (Connect):** Lanza un ciclo de conexión general para que el driver
 * se enlace con los dispositivos compatibles
 *
 * @param[in]  FilePath          Ruta del fichero del driver. Ejemplo: L"\\EFI\\Drivers\\NtfsDxe.efi"
 * @param[out] FileImageHandle   Puntero a una variable EFI_HANDLE que recibirá el
 * handle del driver si la operación tiene éxito
 *
 * @retval EFI_SUCCESS           El driver fue cargado, iniciado y conectado con éxito
 * @retval EFI_INVALID_PARAMETER Uno de los parámetros de entrada es nulo
 * @retval Otros                 Cualquier otro código de error del sistema
 */
EFI_STATUS
UtilsFiles_LoadAndStartImageFile(
    IN CONST CHAR16 *FilePath,
	OUT EFI_HANDLE *FileImageHandle
);


#endif
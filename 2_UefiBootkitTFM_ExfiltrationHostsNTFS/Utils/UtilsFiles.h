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

// ************************************
//     EDK2 Libraries
// ************************************

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


// ************************************
//     Payloads
// ************************************

// Incluimos los datos binarios del driver NTFS que queremos escribir. Este fichero
// define una variable global (un array de bytes) que contiene el driver.
#include <Payloads/PayloadsNtfs.h>



// ============================================================================
// FUNCTIONS
// ============================================================================

/**
* @brief Lee el contenido completo de un fichero y lo devuelve en un buffer.
 *
 * @details
 * Esta función se encarga de todo el proceso de lectura de un fichero:
 * 1. Abre el fichero especificado en modo de solo lectura.
 * 2. Obtiene la información del fichero para averiguar su tamaño.
 * 3. Reserva un buffer en memoria con el tamaño exacto del fichero.
 * 4. Lee todo el contenido del fichero en el buffer.
 * 5. Cierra el fichero y devuelve el buffer y su tamaño.
 *
 * @warning
 * Esta función reserva memoria con `AllocatePool`. Es responsabilidad de
 * la función que la llama liberar esta memoria con `FreePool(*Buffer)`
 * cuando ya no la necesite para evitar memory leaks.
 *
 * @param[in]  VolumeHandle      Handle del volumen donde se encuentra el fichero.
 * @param[in]  FilePath          Ruta del fichero a leer.
 * @param[out] Buffer            Puntero que recibirá la dirección del buffer con
 * el contenido del fichero.
 * @param[out] BufferSize        Puntero que recibirá el tamaño del buffer (y del fichero).
 *
 * @retval EFI_SUCCESS           El fichero se leyó correctamente.
 * @retval Otros                 Cualquier otro código de error del sistema.
 */
EFI_STATUS
UtilsFiles_ReadFile(
    IN  EFI_HANDLE   VolumeHandle,
    IN  CONST CHAR16 *FilePath,
    OUT VOID         **Data,
    OUT UINTN        *DataSize
);


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
);

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
);

#endif
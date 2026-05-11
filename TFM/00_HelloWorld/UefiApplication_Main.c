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


// ============================================================================
// FUNCTIONS
// ============================================================================

EFI_STATUS
EFIAPI
UefiMain(
	IN EFI_HANDLE ImageHandle,
	IN EFI_SYSTEM_TABLE *SystemTable)
{
	Print(L"Hello World from UefiBootkitTFM_HelloWorld (UEFI Application)\r\n");
    return EFI_SUCCESS;
}
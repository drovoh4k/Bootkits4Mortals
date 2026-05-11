//  _____                                                        _____ 
// ( ___ )                                                      ( ___ )
//  |   |~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~|   | 
//  |   |  _   _  _    _  _       ____                   _      |   | 
//  |   | | | | || |_ (_)| | ___ / ___| _ __  __ _ _ __ | |__   |   | 
//  |   | | | | || __|| || |/ __| |  _ | '__|/ _` | '_ \| '_ \  |   | 
//  |   | | |_| || |_ | || |\__ \ |_| || |  | (_| | |_) | | | | |   | 
//  |   |  \___/  \__||_||_||___/\____||_|   \__,_| .__/|_| |_| |   | 
//  |   |                                         |_|            |   | 
//  |___|~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~|___| 
// (_____)                                                      (_____)

#ifndef _UTILS_GRAPHICS_H_
#define _UTILS_GRAPHICS_H_


// ============================================================================
// INCLUDES
// ============================================================================

#include <Uefi.h>
#include <Library/UefiLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/MemoryAllocationLib.h>
#include <IndustryStandard/Bmp.h>
#include <Protocol/GraphicsOutput.h>


// ============================================================================
// FUNCTIONS
// ============================================================================

EFI_STATUS
UtilsGraphics_SetConsoleOutputToHighestTextResolution(VOID);

EFI_STATUS
UtilsGraphics_LocateGraphicsOutputProtocol(
    OUT EFI_GRAPHICS_OUTPUT_PROTOCOL **GraphicsOutput
);

EFI_STATUS
UtilsGraphics_DrawBmpCentered(
    IN EFI_GRAPHICS_OUTPUT_PROTOCOL *GraphicsOutput,
    IN CONST UINT8                  *BmpData,
    IN UINTN                        BmpSize
);

/**
 * @brief Pinta un BMP 24-bit sin compresion dentro de un rectangulo concreto.
 *
 * @details
 * La imagen se escala manteniendo proporcion y se centra dentro del rectangulo
 * indicado por (BoxX, BoxY, BoxWidth, BoxHeight).
 */
EFI_STATUS
UtilsGraphics_DrawBmpScaledInRect(
    IN EFI_GRAPHICS_OUTPUT_PROTOCOL *GraphicsOutput,
    IN CONST UINT8                  *BmpData,
    IN UINTN                        BmpSize,
    IN UINTN                        BoxX,
    IN UINTN                        BoxY,
    IN UINTN                        BoxWidth,
    IN UINTN                        BoxHeight
);

#endif

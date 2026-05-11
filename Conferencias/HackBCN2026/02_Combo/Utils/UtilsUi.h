//  _____                                    _____
// ( ___ )                                  ( ___ )
//  |   |~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~|   |
//  |   |              UtilsUi.h             |   |
//  |___|~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~|___|
// (_____)                                  (_____)

#ifndef _UTILS_UI_H_
#define _UTILS_UI_H_

#include <Uefi.h>
#include <Library/UefiLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Protocol/GraphicsOutput.h>

// ============================================================================
// TIPOS PUBLICOS
// ============================================================================

// Maximo de caracteres en el campo de texto (visible a los llamadores).
#define UI_MAX_INPUT_CHARS 48

// Callback invocado cada vez que el usuario pulsa ENTER en el campo de texto.
// Text    : cadena introducida (terminada en '\0', puede estar vacia).
// Context : puntero opaco pasado desde UI_SCREEN_CONFIG (puede ser NULL).
typedef VOID (EFIAPI *UI_SUBMIT_HANDLER)(
    IN CONST CHAR16 *Text,
    IN VOID         *Context
);

// Una linea de texto del area de contenido.
// Text      : cadena a mostrar. NULL → linea en blanco.
// Highlight : TRUE → amarillo, FALSE → blanco.
typedef struct {
    CONST CHAR16 *Text;
    BOOLEAN       Highlight;
} UI_LINE;

// Configuracion completa de la pantalla.
typedef struct {
    CONST CHAR16        *Title;           // Texto del titulo (caja con =)
    CONST UI_LINE       *Lines;           // Array de lineas de contenido
    UINTN                LineCount;       // Numero de elementos en Lines
    UI_SUBMIT_HANDLER    OnSubmit;        // Callback al pulsar ENTER (puede ser NULL)
    VOID                *OnSubmitContext; // Contexto para OnSubmit (puede ser NULL)

    // Feedback opcional: buffer propiedad del llamador.
    // El callback escribe aqui el mensaje a mostrar tras ENTER.
    // NULL → sin fila de feedback.
    CHAR16              *FeedbackLine;
    UINTN                FeedbackLineLen; // Tamanio del buffer en CHAR16

    // Senal de salida: el callback pone *RequestExit = TRUE para cerrar
    // la pantalla automaticamente despues de mostrar el feedback.
    // NULL → ignorado.
    BOOLEAN             *RequestExit;
} UI_SCREEN_CONFIG;

// ============================================================================
// API PUBLICA
// ============================================================================

EFI_STATUS
UtilsUi_ShowScreen(
    IN EFI_GRAPHICS_OUTPUT_PROTOCOL *GraphicsOutput,
    IN CONST UI_SCREEN_CONFIG       *Config
);

#endif

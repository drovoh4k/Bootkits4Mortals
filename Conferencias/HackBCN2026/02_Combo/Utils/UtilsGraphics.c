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

#include <Utils/UtilsGraphics.h>

STATIC
EFI_STATUS
UtilsGraphics_InternalDrawBmpToRect(
    IN EFI_GRAPHICS_OUTPUT_PROTOCOL *GraphicsOutput,
    IN CONST UINT8                  *BmpData,
    IN UINTN                        BmpSize,
    IN UINTN                        BoxX,
    IN UINTN                        BoxY,
    IN UINTN                        BoxWidth,
    IN UINTN                        BoxHeight,
    IN BOOLEAN                      CenterToFullScreen
)
{
    if (GraphicsOutput == NULL ||
        GraphicsOutput->Mode == NULL ||
        GraphicsOutput->Mode->Info == NULL ||
        BmpData == NULL ||
        BmpSize < sizeof(BMP_IMAGE_HEADER) ||
        BoxWidth == 0 ||
        BoxHeight == 0)
    {
        Print(L"[ ERROR ]-{ UtilsGraphics.c }-( InternalDrawBmpToRect )-> Parametros GOP/BMP invalidos.\n");
        return EFI_INVALID_PARAMETER;
    }

    BMP_IMAGE_HEADER *BmpHeader = (BMP_IMAGE_HEADER *)BmpData;

    if (BmpHeader->CharB != 'B' || BmpHeader->CharM != 'M')
    {
        Print(L"[ ERROR ]-{ UtilsGraphics.c }-( InternalDrawBmpToRect )-> La imagen embebida no tiene firma BMP BM.\n");
        return EFI_UNSUPPORTED;
    }

    if (BmpHeader->BitPerPixel != 24 || BmpHeader->CompressionType != 0)
    {
        Print(L"[ ERROR ]-{ UtilsGraphics.c }-( InternalDrawBmpToRect )-> BMP no soportado. Se espera 24-bit sin compresion.\n");
        return EFI_UNSUPPORTED;
    }

    if (BmpHeader->ImageOffset >= BmpSize)
    {
        Print(L"[ ERROR ]-{ UtilsGraphics.c }-( InternalDrawBmpToRect )-> Offset de pixeles fuera del buffer BMP.\n");
        return EFI_INVALID_PARAMETER;
    }

    UINTN ImageWidth = BmpHeader->PixelWidth;
    UINTN ImageHeight = BmpHeader->PixelHeight;
    UINTN ScreenWidth = GraphicsOutput->Mode->Info->HorizontalResolution;
    UINTN ScreenHeight = GraphicsOutput->Mode->Info->VerticalResolution;

    if (ImageWidth == 0 || ImageHeight == 0 || ScreenWidth == 0 || ScreenHeight == 0)
    {
        return EFI_INVALID_PARAMETER;
    }

    if (BoxX >= ScreenWidth || BoxY >= ScreenHeight)
    {
        return EFI_INVALID_PARAMETER;
    }

    if (BoxX + BoxWidth > ScreenWidth)
    {
        BoxWidth = ScreenWidth - BoxX;
    }

    if (BoxY + BoxHeight > ScreenHeight)
    {
        BoxHeight = ScreenHeight - BoxY;
    }

    if (BoxWidth == 0 || BoxHeight == 0)
    {
        return EFI_INVALID_PARAMETER;
    }

    UINTN TargetWidth = ImageWidth;
    UINTN TargetHeight = ImageHeight;

    if (TargetWidth > BoxWidth || TargetHeight > BoxHeight)
    {
        UINTN WidthBasedHeight = (ImageHeight * BoxWidth) / ImageWidth;
        UINTN HeightBasedWidth = (ImageWidth * BoxHeight) / ImageHeight;

        if (WidthBasedHeight <= BoxHeight)
        {
            TargetWidth = BoxWidth;
            TargetHeight = WidthBasedHeight;
        }
        else
        {
            TargetWidth = HeightBasedWidth;
            TargetHeight = BoxHeight;
        }
    }

    if (TargetWidth == 0) { TargetWidth = 1; }
    if (TargetHeight == 0) { TargetHeight = 1; }

    UINTN ImageX = BoxX + (BoxWidth - TargetWidth) / 2;
    UINTN ImageY = BoxY + (BoxHeight - TargetHeight) / 2;

    if (CenterToFullScreen)
    {
        Print(L"[ INFO ]-{ UtilsGraphics.c }-( DrawBmpCentered )-> Pantalla: %ux%u\n", ScreenWidth, ScreenHeight);
        Print(L"[ INFO ]-{ UtilsGraphics.c }-( DrawBmpCentered )-> Imagen original: %ux%u\n", ImageWidth, ImageHeight);
        Print(L"[ INFO ]-{ UtilsGraphics.c }-( DrawBmpCentered )-> Imagen pintada: %ux%u\n", TargetWidth, TargetHeight);
        Print(L"[ INFO ]-{ UtilsGraphics.c }-( DrawBmpCentered )-> Posicion: X=%u, Y=%u\n", ImageX, ImageY);
    }

    if (TargetWidth > MAX_UINTN / TargetHeight ||
        (TargetWidth * TargetHeight) > MAX_UINTN / sizeof(EFI_GRAPHICS_OUTPUT_BLT_PIXEL))
    {
        return EFI_BAD_BUFFER_SIZE;
    }

    UINTN BltSize = TargetWidth * TargetHeight * sizeof(EFI_GRAPHICS_OUTPUT_BLT_PIXEL);
    EFI_GRAPHICS_OUTPUT_BLT_PIXEL *BltBuffer = AllocateZeroPool(BltSize);
    if (BltBuffer == NULL)
    {
        return EFI_OUT_OF_RESOURCES;
    }

    // Rendimiento: precalculamos los mapas X/Y para evitar divisiones dentro
    // del bucle interno de pixeles. En firmware/VMs lentas esto se nota bastante.
    UINTN *XMap = AllocateZeroPool(TargetWidth * sizeof(UINTN));
    UINTN *YMap = AllocateZeroPool(TargetHeight * sizeof(UINTN));
    if (XMap == NULL || YMap == NULL)
    {
        if (XMap != NULL) { FreePool(XMap); }
        if (YMap != NULL) { FreePool(YMap); }
        FreePool(BltBuffer);
        return EFI_OUT_OF_RESOURCES;
    }

    for (UINTN X = 0; X < TargetWidth; X++)
    {
        XMap[X] = (X * ImageWidth) / TargetWidth;
    }

    for (UINTN Y = 0; Y < TargetHeight; Y++)
    {
        YMap[Y] = ImageHeight - 1 - ((Y * ImageHeight) / TargetHeight);
    }

    UINTN BytesPerPixel = 3;
    UINTN SourceStride = ((ImageWidth * BytesPerPixel + 3) / 4) * 4;

    for (UINTN Y = 0; Y < TargetHeight; Y++)
    {
        UINTN SourceY = YMap[Y];
        UINTN SourceRow = BmpHeader->ImageOffset + SourceY * SourceStride;
        UINTN DestRow = Y * TargetWidth;

        if (SourceRow + ImageWidth * BytesPerPixel > BmpSize)
        {
            FreePool(YMap);
            FreePool(XMap);
            FreePool(BltBuffer);
            return EFI_BAD_BUFFER_SIZE;
        }

        for (UINTN X = 0; X < TargetWidth; X++)
        {
            UINTN SourceIndex = SourceRow + XMap[X] * BytesPerPixel;
            UINTN DestIndex = DestRow + X;

            BltBuffer[DestIndex].Blue = BmpData[SourceIndex + 0];
            BltBuffer[DestIndex].Green = BmpData[SourceIndex + 1];
            BltBuffer[DestIndex].Red = BmpData[SourceIndex + 2];
            BltBuffer[DestIndex].Reserved = 0;
        }
    }

    FreePool(YMap);
    FreePool(XMap);

    EFI_STATUS Status = GraphicsOutput->Blt(
        GraphicsOutput,
        BltBuffer,
        EfiBltBufferToVideo,
        0,
        0,
        ImageX,
        ImageY,
        TargetWidth,
        TargetHeight,
        TargetWidth * sizeof(EFI_GRAPHICS_OUTPUT_BLT_PIXEL)
    );

    if (EFI_ERROR(Status) && CenterToFullScreen)
    {
        Print(L"[ ERROR ]-{ UtilsGraphics.c }-( DrawBmpCentered )-> GOP Blt fallo. Codigo: %r\n", Status);
    }

    FreePool(BltBuffer);
    return Status;
}

EFI_STATUS
UtilsGraphics_SetConsoleOutputToHighestTextResolution(VOID)
{
    if (gST == NULL || gST->ConOut == NULL || gST->ConOut->Mode == NULL)
    {
        return EFI_UNSUPPORTED;
    }

    UINTN MaxMode = gST->ConOut->Mode->MaxMode;
    UINTN BestMode = gST->ConOut->Mode->Mode;
    UINTN BestArea = 0;

    for (UINTN Mode = 0; Mode < MaxMode; Mode++)
    {
        UINTN Columns = 0;
        UINTN Rows = 0;
        EFI_STATUS Status = gST->ConOut->QueryMode(gST->ConOut, Mode, &Columns, &Rows);

        if (!EFI_ERROR(Status) && (Columns * Rows) > BestArea)
        {
            BestArea = Columns * Rows;
            BestMode = Mode;
        }
    }

    return gST->ConOut->SetMode(gST->ConOut, BestMode);
}

EFI_STATUS
UtilsGraphics_LocateGraphicsOutputProtocol(
    OUT EFI_GRAPHICS_OUTPUT_PROTOCOL **GraphicsOutput
)
{
    if (GraphicsOutput == NULL)
    {
        return EFI_INVALID_PARAMETER;
    }

    *GraphicsOutput = NULL;

    return gBS->LocateProtocol(
        &gEfiGraphicsOutputProtocolGuid,
        NULL,
        (VOID **)GraphicsOutput
    );
}

EFI_STATUS
UtilsGraphics_DrawBmpCentered(
    IN EFI_GRAPHICS_OUTPUT_PROTOCOL *GraphicsOutput,
    IN CONST UINT8                  *BmpData,
    IN UINTN                        BmpSize
)
{
    if (GraphicsOutput == NULL || GraphicsOutput->Mode == NULL || GraphicsOutput->Mode->Info == NULL)
    {
        return EFI_INVALID_PARAMETER;
    }

    return UtilsGraphics_InternalDrawBmpToRect(
        GraphicsOutput,
        BmpData,
        BmpSize,
        0,
        0,
        GraphicsOutput->Mode->Info->HorizontalResolution,
        GraphicsOutput->Mode->Info->VerticalResolution,
        TRUE
    );
}

EFI_STATUS
UtilsGraphics_DrawBmpScaledInRect(
    IN EFI_GRAPHICS_OUTPUT_PROTOCOL *GraphicsOutput,
    IN CONST UINT8                  *BmpData,
    IN UINTN                        BmpSize,
    IN UINTN                        BoxX,
    IN UINTN                        BoxY,
    IN UINTN                        BoxWidth,
    IN UINTN                        BoxHeight
)
{
    return UtilsGraphics_InternalDrawBmpToRect(
        GraphicsOutput,
        BmpData,
        BmpSize,
        BoxX,
        BoxY,
        BoxWidth,
        BoxHeight,
        FALSE
    );
}

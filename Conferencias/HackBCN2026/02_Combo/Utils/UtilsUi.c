//  _____                                    _____
// ( ___ )                                  ( ___ )
//  |   |~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~|   |
//  |   |              UtilsUi.c             |   |
//  |___|~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~|___|
// (_____)                                  (_____)

#include <Utils/UtilsUi.h>
#include <Utils/UtilsGraphics.h>
#include <Payloads/PayloadsLogoBmp.h>

STATIC
EFI_GRAPHICS_OUTPUT_BLT_PIXEL
UiColor(
    IN UINT8 Red,
    IN UINT8 Green,
    IN UINT8 Blue
)
{
    EFI_GRAPHICS_OUTPUT_BLT_PIXEL Color;
    Color.Red = Red;
    Color.Green = Green;
    Color.Blue = Blue;
    Color.Reserved = 0;
    return Color;
}

STATIC
EFI_STATUS
UiFillRect(
    IN EFI_GRAPHICS_OUTPUT_PROTOCOL *GraphicsOutput,
    IN UINTN X,
    IN UINTN Y,
    IN UINTN Width,
    IN UINTN Height,
    IN EFI_GRAPHICS_OUTPUT_BLT_PIXEL Color
)
{
    if (GraphicsOutput == NULL || Width == 0 || Height == 0)
    {
        return EFI_INVALID_PARAMETER;
    }

    return GraphicsOutput->Blt(
        GraphicsOutput,
        &Color,
        EfiBltVideoFill,
        0,
        0,
        X,
        Y,
        Width,
        Height,
        0
    );
}

STATIC
EFI_STATUS
UiDrawBorder(
    IN EFI_GRAPHICS_OUTPUT_PROTOCOL *GraphicsOutput,
    IN UINTN X,
    IN UINTN Y,
    IN UINTN Width,
    IN UINTN Height,
    IN UINTN Thickness,
    IN EFI_GRAPHICS_OUTPUT_BLT_PIXEL Color
)
{
    if (Width <= Thickness * 2 || Height <= Thickness * 2)
    {
        return EFI_INVALID_PARAMETER;
    }

    UiFillRect(GraphicsOutput, X, Y, Width, Thickness, Color);
    UiFillRect(GraphicsOutput, X, Y + Height - Thickness, Width, Thickness, Color);
    UiFillRect(GraphicsOutput, X, Y, Thickness, Height, Color);
    UiFillRect(GraphicsOutput, X + Width - Thickness, Y, Thickness, Height, Color);
    return EFI_SUCCESS;
}

STATIC
VOID
UiGetTextGrid(
    OUT UINTN *Columns,
    OUT UINTN *Rows
)
{
    *Columns = 80;
    *Rows = 25;

    if (gST != NULL && gST->ConOut != NULL && gST->ConOut->Mode != NULL)
    {
        UINTN QueryColumns = 0;
        UINTN QueryRows = 0;
        EFI_STATUS Status = gST->ConOut->QueryMode(
            gST->ConOut,
            gST->ConOut->Mode->Mode,
            &QueryColumns,
            &QueryRows
        );

        if (!EFI_ERROR(Status) && QueryColumns > 0 && QueryRows > 0)
        {
            *Columns = QueryColumns;
            *Rows = QueryRows;
        }
    }
}

STATIC
VOID
UiSetReadableTextMode(VOID)
{
    if (gST == NULL || gST->ConOut == NULL || gST->ConOut->Mode == NULL)
    {
        return;
    }

    UINTN MaxMode = gST->ConOut->Mode->MaxMode;
    UINTN Desired[][2] = {
        {80, 25},
        {100, 31},
        {80, 50},
        {100, 37},
        {120, 43}
    };

    for (UINTN DesiredIndex = 0; DesiredIndex < sizeof(Desired) / sizeof(Desired[0]); DesiredIndex++)
    {
        for (UINTN Mode = 0; Mode < MaxMode; Mode++)
        {
            UINTN Columns = 0;
            UINTN Rows = 0;
            EFI_STATUS Status = gST->ConOut->QueryMode(gST->ConOut, Mode, &Columns, &Rows);
            if (!EFI_ERROR(Status) && Columns == Desired[DesiredIndex][0] && Rows == Desired[DesiredIndex][1])
            {
                gST->ConOut->SetMode(gST->ConOut, Mode);
                return;
            }
        }
    }
}

STATIC
VOID
UiPrintAt(
    IN UINTN Column,
    IN UINTN Row,
    IN UINTN Attribute,
    IN CONST CHAR16 *Text
)
{
    if (gST == NULL || gST->ConOut == NULL || Text == NULL)
    {
        return;
    }

    gST->ConOut->SetAttribute(gST->ConOut, Attribute);
    gST->ConOut->SetCursorPosition(gST->ConOut, Column, Row);
    Print(L"%s", Text);
}

STATIC
UINTN
UiStrLen16(
    IN CONST CHAR16 *Text
)
{
    UINTN Length = 0;
    if (Text == NULL)
    {
        return 0;
    }

    while (Text[Length] != L'\0')
    {
        Length++;
    }
    return Length;
}

STATIC
VOID
UiPrintCentered(
    IN UINTN Row,
    IN UINTN Attribute,
    IN CONST CHAR16 *Text,
    IN UINTN Columns
)
{
    UINTN Length = UiStrLen16(Text);
    UINTN Column = 0;

    if (Columns > Length)
    {
        Column = (Columns - Length) / 2;
    }

    UiPrintAt(Column, Row, Attribute, Text);
}

STATIC
VOID
UiClearTextLine(
    IN UINTN Column,
    IN UINTN Row,
    IN UINTN Width,
    IN UINTN Attribute
)
{
    CHAR16 Buffer[160];
    UINTN Index;

    if (Width >= 160)
    {
        Width = 159;
    }

    for (Index = 0; Index < Width; Index++)
    {
        Buffer[Index] = L' ';
    }
    Buffer[Width] = L'\0';

    UiPrintAt(Column, Row, Attribute, Buffer);
}

STATIC
VOID
UiDrawInputBoxText(
    IN UINTN Column,
    IN UINTN Row,
    IN CONST CHAR16 *InputText
)
{
    CHAR16 Buffer[UI_MAX_INPUT_CHARS + 3];
    UINTN Index;

    for (Index = 0; Index < UI_MAX_INPUT_CHARS + 2; Index++)
    {
        Buffer[Index] = L' ';
    }
    Buffer[UI_MAX_INPUT_CHARS + 2] = L'\0';

    if (InputText != NULL)
    {
        for (Index = 0; Index < UI_MAX_INPUT_CHARS && InputText[Index] != L'\0'; Index++)
        {
            Buffer[Index + 1] = InputText[Index];
        }
    }

    UiPrintAt(Column, Row, EFI_BLACK | EFI_BACKGROUND_LIGHTGRAY, Buffer);
}

EFI_STATUS
UtilsUi_ShowScreen(
    IN EFI_GRAPHICS_OUTPUT_PROTOCOL *GraphicsOutput,
    IN CONST UI_SCREEN_CONFIG       *Config
)
{
    if (GraphicsOutput == NULL || GraphicsOutput->Mode == NULL || GraphicsOutput->Mode->Info == NULL
        || Config == NULL || Config->Title == NULL || Config->Lines == NULL)
    {
        return EFI_INVALID_PARAMETER;
    }

    UiSetReadableTextMode();

    UINTN ScreenWidth = GraphicsOutput->Mode->Info->HorizontalResolution;
    UINTN ScreenHeight = GraphicsOutput->Mode->Info->VerticalResolution;

    if (ScreenWidth == 0 || ScreenHeight == 0)
    {
        return EFI_INVALID_PARAMETER;
    }

    EFI_GRAPHICS_OUTPUT_BLT_PIXEL Red = UiColor(238, 58, 42);

    UINTN Columns = 0;
    UINTN Rows    = 0;
    UiGetTextGrid(&Columns, &Rows);

    UINTN MarginX     = ScreenWidth  / 30;
    UINTN MarginY     = ScreenHeight / 30;
    UINTN RowHeightPx = (Rows > 0) ? (ScreenHeight / Rows) : 32;
    if (RowHeightPx < 1) { RowHeightPx = 1; }
    UINTN TitleTopRow = MarginY / RowHeightPx + 1;
    UINTN TitleEndRow = TitleTopRow + 3;

    if (gST != NULL && gST->ConOut != NULL)
    {
        gST->ConOut->EnableCursor(gST->ConOut, FALSE);
    }

    // -------------------------------------------------------------------------
    // Fondo y logo en espacio de pixeles.
    // -------------------------------------------------------------------------

    UINTN InnerX = MarginX;
    UINTN InnerW = ScreenWidth  - MarginX * 2;
    UINTN InnerH = ScreenHeight - MarginY * 2;

    UiFillRect(GraphicsOutput, 0, 0, ScreenWidth, ScreenHeight, Red);

    UINTN ContentBottomReserve = ScreenHeight / 12;
    UINTN ContentTop           = (TitleEndRow + 1) * RowHeightPx;
    UINTN ContentHeight        = (ScreenHeight > ContentTop + ContentBottomReserve + MarginY)
                                 ? ScreenHeight - ContentTop - ContentBottomReserve - MarginY
                                 : InnerH / 3;
    UINTN ContentX    = InnerX + (ScreenWidth / 40);
    UINTN ContentW    = InnerW - (ScreenWidth / 20);
    UINTN TextAreaW   = (ContentW * 70) / 100;
    UINTN LogoAreaW   = ContentW - TextAreaW;
    UINTN ColumnGapPx = ScreenWidth / 50;
    if (TextAreaW > ColumnGapPx) { TextAreaW -= ColumnGapPx; }

    UINTN LogoBoxX = ContentX + TextAreaW + ColumnGapPx;
    UINTN LogoBoxY = ContentTop;
    UINTN LogoBoxW = LogoAreaW;
    UINTN LogoBoxH = ContentHeight;

    UtilsGraphics_DrawBmpScaledInRect(
        GraphicsOutput,
        Global_PayloadsLogoBmp_Image,
        Global_PayloadsLogoBmp_ImageSize,
        LogoBoxX,
        LogoBoxY,
        LogoBoxW,
        LogoBoxH
    );

    // -------------------------------------------------------------------------
    // Distribucion en la cuadricula de texto.
    // -------------------------------------------------------------------------

    UINTN ContentRows = Config->LineCount;
    UINTN BlockRows   = ContentRows + 4;
    if (BlockRows < 8)  { BlockRows = 8; }
    if (BlockRows > 20) { BlockRows = 20; }
    UINTN AvailRows  = Rows > TitleEndRow ? Rows - TitleEndRow : 1;
    UINTN TextTopRow = TitleEndRow + (AvailRows > BlockRows ? (AvailRows - BlockRows) / 2 : 1);
    if (TextTopRow < TitleEndRow + 1) { TextTopRow = TitleEndRow + 1; }

    UINTN InputLabelRow = TextTopRow + ContentRows + 1;
    UINTN InputRow      = InputLabelRow + 2;
    if (InputRow      >= Rows) { InputRow      = Rows - 1; }
    if (InputLabelRow >= Rows) { InputLabelRow = Rows - 3; }

    // Fila de feedback: dos filas bajo el input box
    UINTN FeedbackRow = InputRow + 2;
    if (FeedbackRow >= Rows) { FeedbackRow = Rows - 1; }

    UINTN TextZoneCols = (Columns * 70) / 100;
    UINTN TextLeftCol  = 2;
    UINTN InputWidth   = UI_MAX_INPUT_CHARS + 2;
    UINTN InputColumn  = TextLeftCol;

    if (TextZoneCols > TextLeftCol + InputWidth)
    {
        InputColumn = TextLeftCol + (TextZoneCols - TextLeftCol - InputWidth) / 2;
    }

    // --- Titulo ---
    {
        CONST CHAR16 *ScreenTitle = Config->Title;
        UINTN         InnerLen    = UiStrLen16(ScreenTitle);
        UINTN         BoxedLen    = InnerLen + 4;
        UINTN         TitleBoxCol = TextLeftCol;

        if (TextZoneCols > TextLeftCol + BoxedLen)
        {
            TitleBoxCol = TextLeftCol + (TextZoneCols - TextLeftCol - BoxedLen) / 2;
        }

        CHAR16 SepBuf[80];
        UINTN  SL = BoxedLen < 78 ? BoxedLen : 78;
        for (UINTN I = 0; I < SL; I++) { SepBuf[I] = L'='; }
        SepBuf[SL] = L'\0';

        CHAR16 TitleBuf[80];
        UINTN  TP = 0;
        TitleBuf[TP++] = L'=';
        TitleBuf[TP++] = L' ';
        for (UINTN J = 0; ScreenTitle[J] != L'\0' && TP < 77; J++) {
            TitleBuf[TP++] = ScreenTitle[J];
        }
        TitleBuf[TP++] = L' ';
        TitleBuf[TP++] = L'=';
        TitleBuf[TP]   = L'\0';

        UiPrintAt(TitleBoxCol, TitleTopRow,     EFI_WHITE | EFI_BACKGROUND_RED, SepBuf);
        UiPrintAt(TitleBoxCol, TitleTopRow + 1, EFI_WHITE | EFI_BACKGROUND_RED, TitleBuf);
        UiPrintAt(TitleBoxCol, TitleTopRow + 2, EFI_WHITE | EFI_BACKGROUND_RED, SepBuf);
    }

    // --- Lineas de contenido ---
    for (UINTN I = 0; I < Config->LineCount; I++)
    {
        UINTN LineRow = TextTopRow + I;
        if (LineRow >= InputLabelRow) { break; }
        if (Config->Lines[I].Text != NULL)
        {
            UINTN Attr = Config->Lines[I].Highlight
                         ? (EFI_YELLOW | EFI_BACKGROUND_RED)
                         : (EFI_WHITE  | EFI_BACKGROUND_RED);
            UiPrintAt(TextLeftCol, LineRow, Attr, Config->Lines[I].Text);
        }
    }

    UINTN LabelLen = 14;
    UINTN LabelCol = TextLeftCol;
    if (TextZoneCols > TextLeftCol + LabelLen)
    {
        LabelCol = TextLeftCol + (TextZoneCols - TextLeftCol - LabelLen) / 2;
    }
    UiPrintAt(LabelCol, InputLabelRow, EFI_YELLOW | EFI_BACKGROUND_RED, L"[ ENTER TEXT ]");

    CHAR16 InputText[UI_MAX_INPUT_CHARS + 1];
    UINTN InputLength = 0;
    for (UINTN Index = 0; Index <= UI_MAX_INPUT_CHARS; Index++)
    {
        InputText[Index] = L'\0';
    }

    UiDrawInputBoxText(InputColumn, InputRow, InputText);

    // -------------------------------------------------------------------------
    // Bucle interactivo.
    // -------------------------------------------------------------------------

    while (TRUE)
    {
        EFI_INPUT_KEY Key;
        UINTN EventIndex = 0;

        gBS->WaitForEvent(1, &gST->ConIn->WaitForKey, &EventIndex);
        if (EFI_ERROR(gST->ConIn->ReadKeyStroke(gST->ConIn, &Key)))
        {
            continue;
        }

        if (Key.ScanCode == SCAN_ESC)
        {
            break;
        }

        if (Key.UnicodeChar == CHAR_CARRIAGE_RETURN)
        {
            // Llamar callback con el texto actual
            if (Config->OnSubmit != NULL)
            {
                Config->OnSubmit(InputText, Config->OnSubmitContext);
            }

            // Limpiar campo de texto
            InputLength = 0;
            for (UINTN Index = 0; Index <= UI_MAX_INPUT_CHARS; Index++)
            {
                InputText[Index] = L'\0';
            }
            UiDrawInputBoxText(InputColumn, InputRow, InputText);

            // Mostrar feedback si el callback lo ha rellenado
            if (Config->FeedbackLine != NULL && Config->FeedbackLine[0] != L'\0')
            {
                BOOLEAN IsGood = (Config->RequestExit != NULL) ? *(Config->RequestExit) : FALSE;
                UINTN FeedbackAttr = IsGood
                                     ? (EFI_WHITE | EFI_BACKGROUND_GREEN)
                                     : (EFI_WHITE | EFI_BACKGROUND_BLUE);
                UiClearTextLine(InputColumn, FeedbackRow, 60, EFI_WHITE | EFI_BACKGROUND_RED);
                UiPrintAt(InputColumn, FeedbackRow, FeedbackAttr, Config->FeedbackLine);
            }

            // Salir si el callback lo solicita (pausa para que el usuario lea)
            if (Config->RequestExit != NULL && *(Config->RequestExit))
            {
                gBS->Stall(2 * 1000 * 1000);
                break;
            }
        }
        else if (Key.UnicodeChar == CHAR_BACKSPACE)
        {
            if (InputLength > 0)
            {
                InputLength--;
                InputText[InputLength] = L'\0';
                UiDrawInputBoxText(InputColumn, InputRow, InputText);
            }
        }
        else if (Key.UnicodeChar >= L' ' && Key.UnicodeChar <= L'~')
        {
            if (InputLength < UI_MAX_INPUT_CHARS)
            {
                InputText[InputLength] = Key.UnicodeChar;
                InputLength++;
                InputText[InputLength] = L'\0';
                UiDrawInputBoxText(InputColumn, InputRow, InputText);
            }
        }
    }

    if (gST != NULL && gST->ConOut != NULL)
    {
        gST->ConOut->SetAttribute(gST->ConOut, EFI_LIGHTGRAY | EFI_BACKGROUND_BLACK);
        gST->ConOut->EnableCursor(gST->ConOut, TRUE);
        gST->ConOut->ClearScreen(gST->ConOut);
    }

    return EFI_SUCCESS;
}

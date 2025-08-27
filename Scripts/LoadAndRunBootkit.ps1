<#
.SYNOPSIS
    Instala una entrada de arranque UEFI que apunta a un archivo .efi personalizado.

.DESCRIPTION
    Este script realiza las siguientes acciones de forma automatizada:
    1. Verifica los privilegios de Administrador y los solicita si es necesario.
    2. Comprueba e instala el módulo de PowerShell 'UEFIv2' si es necesario.
    3. Monta la Partición del Sistema EFI.
    4. Copia el archivo .efi a la ruta de destino en la ESP.
    5. Elimina cualquier entrada de arranque UEFI anterior con el mismo nombre.
    6. Crea una nueva entrada de arranque UEFI.
    7. Usa 'bcdedit' para establecer esta nueva entrada para que se ejecute en el siguiente reinicio.
    8. Desmonta la ESP de forma segura.
    9. Reinicia el equipo para aplicar los cambios.
#>


# =================================================================================
# SECCIÓN 1: CONFIGURACIÓN Y PARÁMETROS
# =================================================================================
param (
    [string]$EfiSourceFile = "C:\Bootkit\UefiBootkitTFM_Tester.efi",
    [string]$EfiTargetPath = "\EFI\Boot\Bootkit\UefiBootkitTFM_Tester.efi",
    [string]$UefiEntryName = "Bootkit TFM Loader",
    [string]$MountDriveLetter = "M"
)


# =================================================================================
# SECCIÓN 2: INICIO Y VERIFICACIONES
# =================================================================================
Clear-Host
Write-Host "==============================================" -ForegroundColor Yellow
Write-Host "     Instalador de Entrada de Arranque UEFI" -ForegroundColor Yellow
Write-Host "==============================================" -ForegroundColor Yellow
Write-Host ""

# --- 2.1. Comprobar si se está ejecutando como Administrador ---
Write-Host "[*] Verificando permisos de administrador..."
$principal = [Security.Principal.WindowsPrincipal][Security.Principal.WindowsIdentity]::GetCurrent()
if (-not $principal.IsInRole([Security.Principal.WindowsBuiltInRole]::Administrator)) {
    Write-Host "[!] Permisos insuficientes. El script se reiniciará para solicitar privilegios de administrador..." -ForegroundColor Yellow
    Start-Sleep -Seconds 2
    Start-Process powershell.exe -Verb RunAs -ArgumentList "-NoProfile -File `"$($MyInvocation.MyCommand.Path)`""
    exit
}
Write-Host "[+] Permisos de administrador confirmados." -ForegroundColor Green

# --- 2.2. Comprobar e instalar el módulo UEFIv2 si no está presente ---
Write-Host "[*] Verificando la presencia del módulo 'UEFIv2'..."
if (-not (Get-Module -ListAvailable -Name UEFIv2)) {
    Write-Host "[*] Módulo 'UEFIv2' no encontrado. Intentando instalar desde la Galería de PowerShell..."
    try {
        Install-Module -Name UEFIv2 -Force -Scope AllUsers -AllowClobber -Confirm:$false
        Write-Host "[+] Módulo 'UEFIv2' instalado correctamente." -ForegroundColor Green
    } catch {
        Write-Host "[-] ERROR: Fallo al instalar el módulo 'UEFIv2'." -ForegroundColor Red
        Start-Sleep -Seconds 5
        exit 1
    }
} else {
    Write-Host "[+] El módulo 'UEFIv2' ya está instalado."
}
Import-Module UEFIv2


# =================================================================================
# SECCIÓN 3: MONTAJE DE LA PARTICIÓN EFI Y COPIA DE ARCHIVOS
# =================================================================================
# --- 3.1. Montar la partición del sistema EFI ---
Write-Host "[*] Montando la partición EFI en la unidad ${MountDriveLetter}:"
mountvol.exe "${MountDriveLetter}:" /s
Start-Sleep -Seconds 1

if (-not (Test-Path "${MountDriveLetter}:\")) {
    Write-Host "[-] ERROR: No se pudo montar la partición EFI. Abortando." -ForegroundColor Red
    Start-Sleep -Seconds 5
    exit 1
}
Write-Host "[+] Partición EFI montada correctamente en ${MountDriveLetter}:" -ForegroundColor Green

# --- 3.2. Asegurar que la carpeta de destino existe ---
$targetFolder = Split-Path -Path $EfiTargetPath
$fullTargetPathOnMount = "${MountDriveLetter}:${targetFolder}"
if (-not (Test-Path $fullTargetPathOnMount)) {
    Write-Host "[*] La carpeta de destino no existe. Creando: $fullTargetPathOnMount"
    New-Item -ItemType Directory -Path $fullTargetPathOnMount -Force | Out-Null
}

# --- 3.3. Copiar el archivo .efi a la partición EFI ---
Write-Host "[*] Copiando el archivo EFI a la ESP: $EfiSourceFile -> ${MountDriveLetter}:${EfiTargetPath}"
Copy-Item -Path $EfiSourceFile -Destination "${MountDriveLetter}:${EfiTargetPath}" -Force
Write-Host "[+] Archivo EFI copiado con éxito." -ForegroundColor Green


# =================================================================================
# SECCIÓN 4: GESTIÓN DE LA ENTRADA DE ARRANQUE UEFI
# =================================================================================
# --- 4.1. Eliminar cualquier entrada antigua con el mismo nombre ---
$existingEntry = Get-UEFIBootEntry | Where-Object { $_.Description -eq $UefiEntryName }
if ($null -ne $existingEntry) {
    Write-Host "[*] Se encontró una entrada UEFI existente con el nombre '$UefiEntryName'. Eliminándola..."
    Remove-UEFIBootEntry -Id $existingEntry.Id -Confirm:$false
    Write-Host "[+] Entrada UEFI existente eliminada."
}

# --- 4.2. Crear la nueva entrada de arranque UEFI ---
Write-Host "[*] Creando la nueva entrada de arranque UEFI con el nombre: '$UefiEntryName'"
Add-UEFIBootEntry -Name $UefiEntryName -FilePath $EfiTargetPath
Write-Host "[+] Nueva entrada de arranque UEFI creada." -ForegroundColor Green

# --- 4.3. Establecer la nueva entrada para el próximo arranque (con bcdedit) ---
Write-Host "[*] Configurando '$UefiEntryName' para que se ejecute en el próximo arranque..."
$entryIdentifier = $null
try {
    # Obtenemos la salida de bcdedit como una sola cadena de texto
    $firmwareEntries = bcdedit.exe /enum firmware | Out-String

    # Dividimos la salida en bloques. Cada entrada está separada por dos saltos de línea.
    $entryBlocks = $firmwareEntries -split '(\r?\n){2,}'

    # Recorremos cada bloque para encontrar el que nos interesa
    foreach ($block in $entryBlocks) {
        # Si el bloque contiene nuestra descripción, lo procesamos
        if ($block -like "*description*$UefiEntryName*") {
            # Extraemos la línea del identificador de este bloque
            $identifierLine = $block | Select-String -Pattern 'identifier\s+\{[a-f0-9\-]+\}'
            if ($identifierLine) {
                # Extraemos el GUID usando una expresión regular
                $guidMatch = [regex]::Match($identifierLine, '\{[a-f0-9\-]+\}')
                if ($guidMatch.Success) {
                    $entryIdentifier = $guidMatch.Value
                    break # Salimos del bucle porque ya lo encontramos
                }
            }
        }
    }

    if ($entryIdentifier) {
        # Si encontramos el identificador, lo establecemos para el siguiente arranque
        bcdedit.exe /bootsequence $entryIdentifier | Out-Null
        Write-Host "[+] El sistema arrancará desde la nueva entrada en el siguiente reinicio." -ForegroundColor Green
    } else {
        Write-Host "[!] AVISO: No se pudo encontrar el identificador de la nueva entrada con bcdedit." -ForegroundColor Yellow
    }
} catch {
    Write-Host "[!] AVISO: Ocurrió un error al intentar usar 'bcdedit' para configurar el próximo arranque." -ForegroundColor Yellow
}


# =================================================================================
# SECCIÓN 5: LIMPIEZA Y REINICIO
# =================================================================================
Write-Host "[*] Desmontando la partición EFI de la unidad ${MountDriveLetter}:"
mountvol.exe "${MountDriveLetter}:" /d
Write-Host "[+] Partición EFI desmontada con éxito."

# --- 5.2. Reiniciar el equipo ---
Write-Host ""
Write-Host "*****************************************************************" -ForegroundColor Cyan
Write-Host "* ¡Proceso completado! El sistema se reiniciará en 5 segundos. *" -ForegroundColor Cyan
Write-Host "*****************************************************************" -ForegroundColor Cyan
Start-Sleep -Seconds 5
Restart-Computer -Force

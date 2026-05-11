<#
.SYNOPSIS
    Copia todos los archivos .efi de una carpeta de origen a la Particion del Sistema EFI.

.DESCRIPTION
    Este script realiza las siguientes acciones de forma automatizada:
    1. Verifica los privilegios de Administrador y los solicita si es necesario.
    2. Monta la Particion del Sistema EFI.
    3. Copia TODOS los archivos .efi de la carpeta de origen a \EFI\Boot\Bootkit en la ESP.
    4. Desmonta la ESP de forma segura.
    5. Espera a que el usuario pulse Enter para reiniciar el equipo.
#>


# =================================================================================
# SECCION 1: CONFIGURACION Y PARAMETROS
# =================================================================================
param (
    [string]$EfiSourceFolder  = "C:\Bootkit",
    [string]$EfiTargetFolder  = "\EFI\Boot\Bootkit",
    [string]$MountDriveLetter = "M"
)


# =================================================================================
# SECCION 2: INICIO Y VERIFICACIONES
# =================================================================================
Clear-Host
Write-Host "==============================================" -ForegroundColor Yellow
Write-Host "     Copia de archivos .efi a la ESP" -ForegroundColor Yellow
Write-Host "==============================================" -ForegroundColor Yellow
Write-Host ""

# --- 2.1. Comprobar si se esta ejecutando como Administrador ---
Write-Host "[*] Verificando permisos de administrador..."
$principal = [Security.Principal.WindowsPrincipal][Security.Principal.WindowsIdentity]::GetCurrent()
if (-not $principal.IsInRole([Security.Principal.WindowsBuiltInRole]::Administrator)) {
    Write-Host "[!] Permisos insuficientes. El script se reiniciara para solicitar privilegios de administrador..." -ForegroundColor Yellow
    Start-Sleep -Seconds 2
    Start-Process powershell.exe -Verb RunAs -ArgumentList "-NoProfile -File `"$($MyInvocation.MyCommand.Path)`""
    exit
}
Write-Host "[+] Permisos de administrador confirmados." -ForegroundColor Green

# --- 2.2. Comprobar que la carpeta de origen existe y contiene .efi ---
if (-not (Test-Path $EfiSourceFolder)) {
    Write-Host "[-] ERROR: La carpeta de origen '$EfiSourceFolder' no existe." -ForegroundColor Red
    Start-Sleep -Seconds 5
    exit 1
}

$efiFiles = Get-ChildItem -Path $EfiSourceFolder -Filter *.efi -File
if ($efiFiles.Count -eq 0) {
    Write-Host "[-] ERROR: No se encontro ningun archivo .efi en '$EfiSourceFolder'." -ForegroundColor Red
    Start-Sleep -Seconds 5
    exit 1
}
Write-Host "[+] Se encontraron $($efiFiles.Count) archivo(s) .efi para copiar." -ForegroundColor Green


# =================================================================================
# SECCION 3: MONTAJE DE LA PARTICION EFI Y COPIA DE ARCHIVOS
# =================================================================================
# --- 3.1. Montar la particion del sistema EFI ---
Write-Host "[*] Montando la particion EFI en la unidad ${MountDriveLetter}:"
mountvol.exe "${MountDriveLetter}:" /s
Start-Sleep -Seconds 1

if (-not (Test-Path "${MountDriveLetter}:\")) {
    Write-Host "[-] ERROR: No se pudo montar la particion EFI. Abortando." -ForegroundColor Red
    Start-Sleep -Seconds 5
    exit 1
}
Write-Host "[+] Particion EFI montada correctamente en ${MountDriveLetter}:" -ForegroundColor Green

# --- 3.2. Asegurar que la carpeta de destino existe ---
$fullTargetPathOnMount = "${MountDriveLetter}:${EfiTargetFolder}"
if (-not (Test-Path $fullTargetPathOnMount)) {
    Write-Host "[*] La carpeta de destino no existe. Creando: $fullTargetPathOnMount"
    New-Item -ItemType Directory -Path $fullTargetPathOnMount -Force | Out-Null
}

# --- 3.3. Copiar TODOS los .efi de la carpeta de origen a la particion EFI ---
Write-Host "[*] Copiando archivos .efi desde '$EfiSourceFolder' hacia '$fullTargetPathOnMount'..."
foreach ($file in $efiFiles) {
    try {
        Copy-Item -Path $file.FullName -Destination $fullTargetPathOnMount -Force
        Write-Host "    [+] Copiado: $($file.Name)" -ForegroundColor Green
    } catch {
        Write-Host "    [-] ERROR al copiar $($file.Name): $_" -ForegroundColor Red
    }
}


# =================================================================================
# SECCION 4: LIMPIEZA Y REINICIO
# =================================================================================
Write-Host "[*] Desmontando la particion EFI de la unidad ${MountDriveLetter}:"
mountvol.exe "${MountDriveLetter}:" /d
Write-Host "[+] Particion EFI desmontada con exito." -ForegroundColor Green

Write-Host ""
Write-Host "*****************************************************************" -ForegroundColor Cyan
Write-Host "* Proceso completado!                                           *" -ForegroundColor Cyan
Write-Host "*****************************************************************" -ForegroundColor Cyan
Write-Host ""
Write-Host "Pulsa [ENTER] para reiniciar el equipo..." -ForegroundColor Yellow
Read-Host | Out-Null
Write-Host "[*] Reiniciando el equipo..." -ForegroundColor Cyan
Restart-Computer -Force
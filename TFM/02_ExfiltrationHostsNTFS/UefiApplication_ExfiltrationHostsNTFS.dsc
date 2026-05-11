# .DSC File
# https://github.com/tianocore/tianocore.github.io/wiki/Build-Description-Files#the-dsc-file

# Este archivo describe cómo compilar un paquete, es decir, un conjunto de componentes que se proporcionan juntos. Tenga en cuenta que la compilación necesitará al menos un archivo .DSC para realizarse correctamente.


[Defines]
	PLATFORM_NAME               = UefiBootkitTFM_ExfiltrationHostsNTFS
	PLATFORM_GUID               = 00000000-0000-0000-0000-000000000000 # Revisar http://www.guidgen.com/
	PLATFORM_VERSION            = 1.00
	DSC_SPECIFICATION           = 1.26
	OUTPUT_DIRECTORY            = Build/UefiBootkitTFM_ExfiltrationHostsNTFS
	SUPPORTED_ARCHITECTURES     = X64
	BUILD_TARGETS               = DEBUG|RELEASE
	SKUID_IDENTIFIER            = DEFAULT


# Enumera las distintas bibliotecas que pueden utilizar los componentes de este paquete. Esto indica al sistema de compilación dónde se encuentra la biblioteca con la que se debe vincular. El archivo .INF de los módulos indica LibNameToReference en su sección [LibraryClasses] y el sistema de compilación busca en esta sección cómo encontrarla. El sistema de compilación no utiliza la sección [Packages] del archivo .INF para encontrar la biblioteca con la que enlazar; utiliza la sección [Packages] para encontrar la ubicación de los archivos de encabezado de una biblioteca en un archivo .DEC de paquetes.
[LibraryClasses]
	# Entry point
	UefiApplicationEntryPoint | MdePkg/Library/UefiApplicationEntryPoint/UefiApplicationEntryPoint.inf
		# Biblioteca de puntos de entrada de módulos para aplicaciones UEFI
		# https://github.com/tianocore/edk2/blob/master/MdePkg/Include/Library/UefiApplicationEntryPoint.h
		# https://github.com/tianocore/edk2/blob/master/MdePkg/Library/UefiApplicationEntryPoint/UefiApplicationEntryPoint.inf
	
	# Tables
	UefiBootServicesTableLib | MdePkg/Library/UefiBootServicesTableLib/UefiBootServicesTableLib.inf
		# Proporciona un servicio para recuperar un puntero a la tabla de servicios de arranque EFI. Solo disponible para los tipos de módulos DXE y UEFI.
		# https://github.com/tianocore/edk2/blob/master/MdePkg/Include/Library/UefiBootServicesTableLib.h
		# https://github.com/tianocore/edk2/blob/master/MdePkg/Library/UefiBootServicesTableLib/UefiBootServicesTableLib.inf
	UefiRuntimeServicesTableLib | MdePkg/Library/UefiRuntimeServicesTableLib/UefiRuntimeServicesTableLib.inf
		# Proporciona un servicio para recuperar un puntero a la tabla de servicios de tiempo de ejecución EFI
		# https://github.com/tianocore/edk2/blob/master/MdePkg/Include/Library/UefiRuntimeServicesTableLib.h
		# https://github.com/tianocore/edk2/blob/master/MdePkg/Library/UefiRuntimeServicesTableLib/UefiRuntimeServicesTableLib.inf
	
	# Base
	UefiLib | MdePkg/Library/UefiLib/UefiLib.inf
		# Proporciona funciones de biblioteca para operaciones UEFI comunes. Solo disponible para los tipos de módulos DXE y UEFI.
		# https://github.com/tianocore/edk2/blob/master/MdePkg/Include/Library/UefiLib.h
		# https://github.com/tianocore/edk2/blob/master/MdePkg/Library/UefiLib/UefiLib.inf
	PrintLib | MdePkg/Library/BasePrintLib/BasePrintLib.inf
		# Proporciona servicios para imprimir una cadena formateada en un búfer. Se admiten todas las combinaciones de cadenas Unicode y ASCII.
		# https://github.com/tianocore/edk2/blob/master/MdePkg/Include/Library/PrintLib.h
		# https://github.com/tianocore/edk2/blob/master/MdePkg/Library/BasePrintLib/BasePrintLib.inf
	DebugLib | MdePkg/Library/BaseDebugLibNull/BaseDebugLibNull.inf
		# Proporciona servicios para imprimir una cadena formateada en un búfer. Se admiten todas las combinaciones de cadenas Unicode y ASCII.
		# https://github.com/tianocore/edk2/blob/master/MdePkg/Include/Library/DebugLib.h
		# https://github.com/tianocore/edk2/blob/master/MdePkg/Library/BaseDebugLibNull/BaseDebugLibNull.inf
	BaseLib | MdePkg/Library/BaseLib/BaseLib.inf
		# Proporciona funciones de cadenas, funciones de listas enlazadas, funciones matemáticas, funciones de sincronización, funciones de rutas de archivos y funciones específicas de la arquitectura de la CPU.
		# https://github.com/tianocore/edk2/blob/master/MdePkg/Include/Library/BaseLib.h
		# https://github.com/tianocore/edk2/blob/master/MdePkg/Library/BaseLib/BaseLib.inf
	
	# Memory
	BaseMemoryLib | MdePkg/Library/BaseMemoryLib/BaseMemoryLib.inf
		# Proporciona funciones de copiar memoria, rellenar memoria, limpiar memoria y GUID
		# https://github.com/tianocore/edk2/blob/master/MdePkg/Include/Library/BaseMemoryLib.h
		# https://github.com/tianocore/edk2/blob/master/MdePkg/Library/BaseMemoryLib/BaseMemoryLib.inf
	MemoryAllocationLib | MdePkg/Library/UefiMemoryAllocationLib/UefiMemoryAllocationLib.inf
		# Proporciona servicios para asignar y liberar búferes de memoria de varios tipos y alineaciones.
		# https://github.com/tianocore/edk2/blob/master/MdePkg/Include/Library/MemoryAllocationLib.h
		# https://github.com/tianocore/edk2/blob/master/MdePkg/Library/UefiMemoryAllocationLib/UefiMemoryAllocationLib.inf
	
	# Files
	DevicePathLib | MdePkg/Library/UefiDevicePathLibDevicePathProtocol/UefiDevicePathLibDevicePathProtocol.inf
		# Provides library functions to construct and parse UEFI Device Paths.
		# https://github.com/tianocore/edk2/blob/master/MdePkg/Include/Library/DevicePathLib.h
		# https://github.com/tianocore/edk2/blob/master/MdePkg/Library/UefiDevicePathLibDevicePathProtocol/UefiDevicePathLibDevicePathProtocol.inf
	FileHandleLib | MdePkg/Library/UefiFileHandleLib/UefiFileHandleLib.inf
		# Proporciona funciones de biblioteca para construir y analizar rutas de dispositivos UEFI.
		# https://github.com/tianocore/edk2/blob/master/MdePkg/Include/Library/FileHandleLib.h
		# https://github.com/tianocore/edk2/blob/master/MdePkg/Library/UefiFileHandleLib/UefiFileHandleLib.inf
	
	# Stack Cookies
	StackCheckLib | MdePkg/Library/StackCheckLib/StackCheckLib.inf
		# Esta biblioteca proporciona funciones de comprobación de cookies de pila para los símbolos insertados por el compilador. Este encabezado no está pensado para ser utilizado directamente por los módulos, sino que define las interfaces esperadas para cada compilador compatible, de modo que si se actualiza la interfaz del compilador sea más fácil realizar un seguimiento.
		# https://github.com/tianocore/edk2/blob/master/MdePkg/Include/Library/StackCheckLib.h
		# https://github.com/tianocore/edk2/blob/master/MdePkg/Library/StackCheckLib/StackCheckLib.inf
	StackCheckFailureHookLib | MdePkg/Library/StackCheckFailureHookLibNull/StackCheckFailureHookLibNull.inf
		# La biblioteca proporciona un gancho que se activa cuando falla la comprobación de una cookie de pila.
		# https://github.com/tianocore/edk2/blob/master/MdePkg/Include/Library/StackCheckFailureHookLib.h
		# https://github.com/tianocore/edk2/blob/master/MdePkg/Library/StackCheckFailureHookLibNull/StackCheckFailureHookLibNull.inf
	
	# Other
	PcdLib | MdePkg/Library/DxePcdLib/DxePcdLib.inf
		# Proporciona servicios de biblioteca para obtener y establecer entradas de la base de datos de configuración de la plataforma.
		# https://github.com/tianocore/edk2/blob/master/MdePkg/Include/Library/PcdLib.h
		# https://github.com/tianocore/edk2/blob/master/MdePkg/Library/DxePcdLib/DxePcdLib.inf
	RegisterFilterLib | MdePkg/Library/RegisterFilterLibNull/RegisterFilterLibNull.inf
		# Archivo público incluido para el filtro de registros IO/MMIO/MSR del puerto.
		# https://github.com/tianocore/edk2/blob/master/MdePkg/Include/Library/RegisterFilterLib.h
		# https://github.com/tianocore/edk2/blob/master/MdePkg/Library/RegisterFilterLibNull/RegisterFilterLibNull.inf
	
	# Shell
	UefiHiiServicesLib | MdeModulePkg/Library/UefiHiiServicesLib/UefiHiiServicesLib.inf
		# Implementación de la biblioteca de servicios UEFI HII.
		# https://github.com/tianocore/edk2/blob/master/MdeModulePkg/Library/UefiHiiServicesLib/UefiHiiServicesLib.inf
	SortLib | MdeModulePkg/Library/UefiSortLib/UefiSortLib.inf
		# Biblioteca utilizada para rutinas de clasificación.
		# https://github.com/tianocore/edk2/blob/master/MdeModulePkg/Library/UefiSortLib/UefiSortLib.inf
	HiiLib | MdeModulePkg/Library/UefiHiiLib/UefiHiiLib.inf
		# Implementación de la biblioteca HII utilizando protocolos y servicios UEFI HII.
		# https://github.com/tianocore/edk2/blob/master/MdeModulePkg/Library/UefiHiiLib/UefiHiiLib.inf
	ShellLib|ShellPkg/Library/UefiShellLib/UefiShellLib.inf
		# Proporciona una interfaz para shell para comandos y aplicaciones del UEFI Shell.
		# https://github.com/tianocore/edk2/blob/master/ShellPkg/Library/UefiShellLib/UefiShellLib.inf


# Enumera los distintos componentes o módulos que se deben compilar para este paquete, compilados específicamente como resultado de la compilación del archivo .DSC del paquete, no cuando el paquete se referencia en la sección [Packages] de un archivo .INF. También se pueden especificar secciones específicas de la arquitectura añadiendo un punto y la arquitectura al final de Components (por ejemplo, [Components.IA32]). Esta sección también se puede utilizar para compilar una biblioteca a la que se hace referencia en la sección [Packages] de un archivo .INF.    Esto se utiliza cuando se desea compilar una biblioteca independiente y vincularla de forma tradicional o para depurar la biblioteca y asegurarse de que se compila correctamente. Debe haber al menos un archivo .inf en la sección de componentes para que la compilación se realice correctamente. Nota: la ruta relativa es desde el directorio base edk2 y no desde el directorio del paquete (también denominado directorio del espacio de trabajo).
[Components]
	UefiBootkitTFM_ExfiltrationHostsNTFS/UefiApplication_ExfiltrationHostsNTFS.inf  # UEFI Application

# .DSC File
# https://github.com/tianocore/tianocore.github.io/wiki/Build-Description-Files#the-dsc-file


[Defines]
	PLATFORM_NAME               = UefiApplication_Combo
	PLATFORM_GUID               = ca11ab1e-cafe-beef-dead-c0debabe0002
	PLATFORM_VERSION            = 1.00
	DSC_SPECIFICATION           = 1.26
	OUTPUT_DIRECTORY            = Build/UefiApplication_Combo
	SUPPORTED_ARCHITECTURES     = X64|AARCH64
	BUILD_TARGETS               = DEBUG|RELEASE
	SKUID_IDENTIFIER            = DEFAULT


[LibraryClasses]
	# Entry point
	UefiApplicationEntryPoint | MdePkg/Library/UefiApplicationEntryPoint/UefiApplicationEntryPoint.inf

	# Tables
	UefiBootServicesTableLib  | MdePkg/Library/UefiBootServicesTableLib/UefiBootServicesTableLib.inf
	UefiRuntimeServicesTableLib | MdePkg/Library/UefiRuntimeServicesTableLib/UefiRuntimeServicesTableLib.inf

	# Base
	UefiLib               | MdePkg/Library/UefiLib/UefiLib.inf
	PrintLib              | MdePkg/Library/BasePrintLib/BasePrintLib.inf
	DebugLib              | MdePkg/Library/BaseDebugLibNull/BaseDebugLibNull.inf
	BaseLib               | MdePkg/Library/BaseLib/BaseLib.inf
	BaseMemoryLib         | MdePkg/Library/BaseMemoryLib/BaseMemoryLib.inf
	MemoryAllocationLib   | MdePkg/Library/UefiMemoryAllocationLib/UefiMemoryAllocationLib.inf

	# Files / Device paths
	DevicePathLib         | MdePkg/Library/UefiDevicePathLibDevicePathProtocol/UefiDevicePathLibDevicePathProtocol.inf
	FileHandleLib         | MdePkg/Library/UefiFileHandleLib/UefiFileHandleLib.inf

	# Stack Cookies
	StackCheckLib         | MdePkg/Library/StackCheckLib/StackCheckLib.inf
	StackCheckFailureHookLib | MdePkg/Library/StackCheckFailureHookLibNull/StackCheckFailureHookLibNull.inf

	# Other
	PcdLib                | MdePkg/Library/DxePcdLib/DxePcdLib.inf
	RegisterFilterLib     | MdePkg/Library/RegisterFilterLibNull/RegisterFilterLibNull.inf

	# Shell (requerido por UtilsFiles — ShellOpenFileByName)
	UefiHiiServicesLib    | MdeModulePkg/Library/UefiHiiServicesLib/UefiHiiServicesLib.inf
	SortLib               | MdeModulePkg/Library/UefiSortLib/UefiSortLib.inf
	HiiLib                | MdeModulePkg/Library/UefiHiiLib/UefiHiiLib.inf
	ShellLib              | ShellPkg/Library/UefiShellLib/UefiShellLib.inf


[Components]
	2_UefiApplication_Combo/UefiApplication_Combo.inf

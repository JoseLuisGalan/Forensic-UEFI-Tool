## @file
#  Forensic UEFI Boot Tool -- Descripcion de compilacion del paquete
#
#  SPDX-License-Identifier: BSD-2-Clause-Patent
##

################################################################################
[Defines]
################################################################################

  PLATFORM_NAME           = ForensicPkg
  PLATFORM_GUID           = B2C3D4E5-F6A7-8901-BCDE-F12345678901
  PLATFORM_VERSION        = 1.0
  DSC_SPECIFICATION       = 0x00010006
  OUTPUT_DIRECTORY        = Build/ForensicPkg
  SUPPORTED_ARCHITECTURES = X64
  BUILD_TARGETS           = DEBUG|RELEASE
  SKUID_IDENTIFIER        = DEFAULT

################################################################################
[LibraryClasses]
################################################################################

  # Punto de entrada para aplicaciones UEFI
  UefiApplicationEntryPoint|MdePkg/Library/UefiApplicationEntryPoint/UefiApplicationEntryPoint.inf

  # Comprobacion de pila (requerida por UefiApplicationEntryPoint en EDK II reciente)
  StackCheckLib|MdePkg/Library/StackCheckLibNull/StackCheckLibNull.inf

  # Servicios UEFI
  UefiLib|MdePkg/Library/UefiLib/UefiLib.inf
  UefiBootServicesTableLib|MdePkg/Library/UefiBootServicesTableLib/UefiBootServicesTableLib.inf
  UefiRuntimeServicesTableLib|MdePkg/Library/UefiRuntimeServicesTableLib/UefiRuntimeServicesTableLib.inf

  # Memoria
  MemoryAllocationLib|MdePkg/Library/UefiMemoryAllocationLib/UefiMemoryAllocationLib.inf
  BaseMemoryLib|MdePkg/Library/BaseMemoryLib/BaseMemoryLib.inf

  # Librerias base
  BaseLib|MdePkg/Library/BaseLib/BaseLib.inf
  PrintLib|MdePkg/Library/BasePrintLib/BasePrintLib.inf
  DevicePathLib|MdePkg/Library/UefiDevicePathLib/UefiDevicePathLib.inf

  # Sincronizacion (requerida por BaseCryptLib)
  SynchronizationLib|MdePkg/Library/BaseSynchronizationLib/BaseSynchronizationLib.inf

  # Timer (requerido por BaseSynchronizationLib)
  TimerLib|MdePkg/Library/BaseTimerLibNullTemplate/BaseTimerLibNullTemplate.inf

  # Matematicas enteras seguras (requerida por CryptoPkg reciente)
  SafeIntLib|MdePkg/Library/BaseSafeIntLib/BaseSafeIntLib.inf

  # Criptografia SHA-256 (CryptoPkg)
  BaseCryptLib|CryptoPkg/Library/BaseCryptLib/BaseCryptLib.inf
  OpensslLib|CryptoPkg/Library/OpensslLib/OpensslLib.inf
  IntrinsicLib|CryptoPkg/Library/IntrinsicLib/IntrinsicLib.inf
  RngLib|MdePkg/Library/BaseRngLib/BaseRngLib.inf

  # Soporte general
  PcdLib|MdePkg/Library/BasePcdLibNull/BasePcdLibNull.inf
  DebugLib|MdePkg/Library/BaseDebugLibNull/BaseDebugLibNull.inf
  DebugPrintErrorLevelLib|MdePkg/Library/BaseDebugPrintErrorLevelLib/BaseDebugPrintErrorLevelLib.inf
  RegisterFilterLib|MdePkg/Library/RegisterFilterLibNull/RegisterFilterLibNull.inf

  # HII (requerida internamente por UefiLib)
  HiiLib|MdeModulePkg/Library/UefiHiiLib/UefiHiiLib.inf
  UefiHiiServicesLib|MdeModulePkg/Library/UefiHiiServicesLib/UefiHiiServicesLib.inf

################################################################################
[Components]
################################################################################

  ForensicPkg/Menu_kit/Menu_kit.inf
  ForensicPkg/Files_shows/Files_shows.inf
  ForensicPkg/Show_info/Show_info.inf
  ForensicPkg/Forense_tool/Forense_tool.inf

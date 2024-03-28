#pragma once

#include <PiDxe.h>
#include <Uefi.h>
#include <Protocol/UgaDraw.h>
#include <Protocol/GraphicsOutput.h>
#include <Protocol/PciIo.h>
#include <Protocol/DriverSupportedEfiVersion.h>
#include <Protocol/EdidOverride.h>
#include <Protocol/EdidDiscovered.h>
#include <Protocol/EdidActive.h>
#include <Protocol/DevicePath.h>

#include <Library/BaseLib.h>
#include <Library/HobLib.h>
#include <Library/DebugLib.h>
#include <Library/UefiDriverEntryPoint.h>
#include <Library/UefiLib.h>
#include <Library/PcdLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/FrameBufferBltLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/DevicePathLib.h>
#include <Library/TimerLib.h>
#include <Guid/GraphicsInfoHob.h>

#include <IndustryStandard/Pci.h>

#include "VirtualBoxDebugLog.h"

#if 1
  #define DbgPrint(a) VBoxDebugPrint a
#else
  #define DbgPrint(a)
#endif


#define SOFT_GPU_VENDOR_ID (0xFFFD)
#define SOFT_GPU_DEVICE_ID (0x0001)

typedef struct {
  UINT32 ModeNumber;
  UINT32 HorizontalResolution;
  UINT32 VerticalResolution;
  UINT32 ColorDepth;
  UINT32 RefreshRate;
} SOFT_GPU_VGA_DATA;

#define GRAPHICS_OUTPUT_INVALID_MODE_NUMBER 0xffff

typedef struct 
{
    UINT32                               Signature;
    EFI_HANDLE                           Handle;
    EFI_HANDLE                           GraphicsOutputHandle;
    EFI_GRAPHICS_OUTPUT_PROTOCOL         GraphicsOutput;
    // EFI_GRAPHICS_OUTPUT_PROTOCOL_MODE    GraphicsOutputMode;
    EFI_DEVICE_PATH_PROTOCOL             *DevicePath;
    EFI_PCI_IO_PROTOCOL                  *PciIo;
    UINT64                               OriginalPciAttributes;
    EFI_EDID_DISCOVERED_PROTOCOL         EdidDiscovered;
    EFI_EDID_ACTIVE_PROTOCOL             EdidActive;
    UINTN                                CurrentMode;
    UINTN                                MaxMode;
    SOFT_GPU_VGA_DATA                    *ModeData;
    BOOLEAN                              NeedsStart;
    UINT8                                BarIndexFrameBuffer;
    UINT16                               DeviceType;
} SOFT_GPU_PRIVATE_DATA;


#define SOFT_GPU_PRIVATE_DATA_SIGNATURE  SIGNATURE_32 ('s', 'g', 'o', 'p')
#define SOFT_GPU_PRIVATE_FROM_THIS(a) \
  CR(a, SOFT_GPU_PRIVATE_DATA, GraphicsOutput, SOFT_GPU_PRIVATE_DATA_SIGNATURE)
  
extern EFI_COMPONENT_NAME_PROTOCOL                gSoftGpuGopComponentName;
extern EFI_COMPONENT_NAME2_PROTOCOL               gSoftGpuGopComponentName2;
extern EFI_DRIVER_BINDING_PROTOCOL                gSoftGpuGopDriverBinding;
extern EFI_DRIVER_SUPPORTED_EFI_VERSION_PROTOCOL  gSoftGpuGopDriverSupportedEfiVersion;

EFI_STATUS
SoftGpuVideoModeSetup(
    SOFT_GPU_PRIVATE_DATA* Private
);

EFI_STATUS
SoftGpuGopConstructor(
    SOFT_GPU_PRIVATE_DATA* Private
);

EFI_STATUS
SoftGpuGopDestructor(
    SOFT_GPU_PRIVATE_DATA* Private
);

EFI_STATUS
EFIAPI
SoftGpuGopComponentNameGetDriverName(
  IN  EFI_COMPONENT_NAME_PROTOCOL  *This,
  IN  CHAR8                        *Language,
  OUT CHAR16                       **DriverName
);

EFI_STATUS
EFIAPI
SoftGpuGopComponentNameGetControllerName(
  IN  EFI_COMPONENT_NAME_PROTOCOL  *This,
  IN  EFI_HANDLE                   ControllerHandle,
  IN  EFI_HANDLE                   ChildHandle        OPTIONAL,
  IN  CHAR8                        *Language,
  OUT CHAR16                       **ControllerName
);

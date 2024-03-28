#include "SoftGpuGop.h"

//
// EFI Component Name Protocol
//
GLOBAL_REMOVE_IF_UNREFERENCED EFI_COMPONENT_NAME_PROTOCOL gSoftGpuGopComponentName = {
  SoftGpuGopComponentNameGetDriverName,
  SoftGpuGopComponentNameGetControllerName,
  "eng"
};

//
// EFI Component Name 2 Protocol
//
GLOBAL_REMOVE_IF_UNREFERENCED EFI_COMPONENT_NAME2_PROTOCOL gSoftGpuGopComponentName2 = {
  (EFI_COMPONENT_NAME2_GET_DRIVER_NAME) SoftGpuGopComponentNameGetDriverName,
  (EFI_COMPONENT_NAME2_GET_CONTROLLER_NAME) SoftGpuGopComponentNameGetControllerName,
  "en"
};

GLOBAL_REMOVE_IF_UNREFERENCED EFI_UNICODE_STRING_TABLE mSoftGpuGopDriverNameTable[] = {
  {
    "eng;en",
    (CHAR16 *)L"SoftGpu GOP Driver"
  },
  {
    NULL,
    NULL
  }
};

GLOBAL_REMOVE_IF_UNREFERENCED EFI_UNICODE_STRING_TABLE mSoftGpuGopControllerNameTable[] = {
  {
    "eng;en",
    (CHAR16 *)L"SoftGpu"
  },
  {
    NULL,
    NULL
  }
};

EFI_STATUS
EFIAPI
SoftGpuGopComponentNameGetDriverName(
  IN  EFI_COMPONENT_NAME_PROTOCOL  *This,
  IN  CHAR8                        *Language,
  OUT CHAR16                       **DriverName
)
{
    return LookupUnicodeString2(
        Language,
        This->SupportedLanguages,
        mSoftGpuGopDriverNameTable,
        DriverName,
        (BOOLEAN)(This == &gSoftGpuGopComponentName)
    );
}

EFI_STATUS
EFIAPI
SoftGpuGopComponentNameGetControllerName(
  IN  EFI_COMPONENT_NAME_PROTOCOL  *This,
  IN  EFI_HANDLE                   ControllerHandle,
  IN  EFI_HANDLE                   ChildHandle        OPTIONAL,
  IN  CHAR8                        *Language,
  OUT CHAR16                       **ControllerName
)
{
    //
    // ChildHandle must be NULL for a Device Driver
    //
    if(ChildHandle != NULL) 
    {
        DbgPrint((DEBUG_INFO, "[%a]: Child Handle (%p) was not null. This must be null for a Device Driver.\n", gEfiCallerBaseName, ChildHandle));
        return EFI_UNSUPPORTED;
    }
  
    //
    // Make sure this driver is currently managing ControllerHandle
    //
    EFI_STATUS Status = EfiTestManagedDevice(
        ControllerHandle,
        gSoftGpuGopDriverBinding.DriverBindingHandle,
        &gEfiPciIoProtocolGuid
    );

    if(EFI_ERROR(Status))
    {
        DbgPrint((DEBUG_INFO, "[%a]: The driver is not currently managed by the ControllerHandle.\n", gEfiCallerBaseName));
        return Status;
    }

    return LookupUnicodeString2(
        Language,
        This->SupportedLanguages,
        mSoftGpuGopControllerNameTable,
        ControllerName,
        (BOOLEAN)(This == &gSoftGpuGopComponentName)
    );
}

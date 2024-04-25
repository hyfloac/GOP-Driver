#include "SoftGpuGop.h"

// CONST GRAPHICS_OUTPUT_PRIVATE_DATA  mGraphicsOutputInstanceTemplate = {
//   GRAPHICS_OUTPUT_PRIVATE_DATA_SIGNATURE,          // Signature
//   NULL,                                            // GraphicsOutputHandle
//   {
//     // GraphicsOutputQueryMode,
//     // GraphicsOutputSetMode,
//     // GraphicsOutputBlt,
//     NULL,
//     NULL,
//     NULL,
//     NULL                                           // Mode
//   },
//   {
//     1,                                             // MaxMode
//     0,                                             // Mode
//     NULL,                                          // Info
//     sizeof (EFI_GRAPHICS_OUTPUT_MODE_INFORMATION), // SizeOfInfo
//     0,                                             // FrameBufferBase
//     0                                              // FrameBufferSize
//   },
//   NULL,                                            // DevicePath
//   NULL,                                            // PciIo
//   0,                                               // PciAttributes
//   NULL,                                            // FrameBufferBltLibConfigure
//   0                                                // FrameBufferBltLibConfigureSize
// };

CONST ACPI_ADR_DEVICE_PATH  mGraphicsOutputAdrNode = {
  {
    ACPI_DEVICE_PATH,
    ACPI_ADR_DP,
    { sizeof (ACPI_ADR_DEVICE_PATH), 0 },
  },
  ACPI_DISPLAY_ADR (1, 0, 0, 1, 0, ACPI_ADR_DISPLAY_TYPE_VGA, 0, 0)
};

// EFI_PEI_GRAPHICS_DEVICE_INFO_HOB  mDefaultGraphicsDeviceInfo = {
//   MAX_UINT16, MAX_UINT16, MAX_UINT16, MAX_UINT16, MAX_UINT8, MAX_UINT8
// };

BOOLEAN  mDriverStarted = FALSE;

EFI_STATUS
EFIAPI
SoftGpuGopDriverBindingSupported(
    IN EFI_DRIVER_BINDING_PROTOCOL  *This,
    IN EFI_HANDLE                   Controller,
    IN EFI_DEVICE_PATH_PROTOCOL     *RemainingDevicePath
);

EFI_STATUS
EFIAPI
SoftGpuGopDriverBindingStart(
    IN EFI_DRIVER_BINDING_PROTOCOL  *This,
    IN EFI_HANDLE                   Controller,
    IN EFI_DEVICE_PATH_PROTOCOL     *RemainingDevicePath
);
  
EFI_STATUS
EFIAPI
SoftGpuGopDriverBindingStop(
    IN EFI_DRIVER_BINDING_PROTOCOL  *This,
    IN EFI_HANDLE                   Controller,
    IN UINTN                        NumberOfChildren,
    IN EFI_HANDLE                   *ChildHandleBuffer
);

EFI_DRIVER_BINDING_PROTOCOL  gSoftGpuGopDriverBinding = {
    SoftGpuGopDriverBindingSupported,
    SoftGpuGopDriverBindingStart,
    SoftGpuGopDriverBindingStop,
    0x10,
    NULL,
    NULL
};

EFI_STATUS
EFIAPI
SoftGpuGopDriverBindingSupported(
    IN EFI_DRIVER_BINDING_PROTOCOL  *This,
    IN EFI_HANDLE                   Controller,
    IN EFI_DEVICE_PATH_PROTOCOL     *RemainingDevicePath
)
{
    if(mDriverStarted)
    {
        return EFI_ALREADY_STARTED;
    }

    // EFI_DEVICE_PATH_PROTOCOL* DevicePath;
    
    EFI_PCI_IO_PROTOCOL* PciIo;

    //
    // Open the PCI I/O Protocol
    //
    EFI_STATUS Status = gBS->OpenProtocol(
        Controller,
        &gEfiPciIoProtocolGuid,
        (VOID**) &PciIo,
        This->DriverBindingHandle,
        Controller,
        EFI_OPEN_PROTOCOL_BY_DRIVER
    );

    // if(Status == EFI_ALREADY_STARTED)
    // {
    //     DbgPrint((DEBUG_INFO, "[%a]: %a:%d PciIo protocol already started. Apparently this is a good thing.\n", gEfiCallerBaseName, __FILE__, __LINE__));
    //     return EFI_SUCCESS;
    // }

    if(EFI_ERROR(Status))
    {
        DbgPrint((DEBUG_INFO, "[%a]: %a:%d Error opening PciIo protocol: %r.\n", gEfiCallerBaseName, __FILE__, __LINE__, Status));
        return Status;
    }

    PCI_TYPE00 Pci;
    EFI_DEV_PATH *Node;
    
    Status = PciIo->Pci.Read(
        PciIo,
        EfiPciIoWidthUint32,
        0,
        sizeof(Pci) / sizeof(UINT32),
        &Pci
    );

    if(EFI_ERROR(Status))
    {
        DbgPrint((DEBUG_INFO, "[%a]: %a:%d Error reading PCI Config: %r\n", gEfiCallerBaseName, __FILE__, __LINE__, Status));
        goto Done;
    }

    if(Pci.Hdr.VendorId == SOFT_GPU_VENDOR_ID && Pci.Hdr.DeviceId == SOFT_GPU_DEVICE_ID) 
    {
        Status = EFI_SUCCESS;

        if(RemainingDevicePath != NULL)
        {
            Node = (EFI_DEV_PATH *) RemainingDevicePath;

            //
            // Check if RemainingDevicePath is the End of Device Path Node,
            // if yes, return EFI_SUCCESS
            //
            if(!IsDevicePathEnd(Node))
            {
                //
                // If RemainingDevicePath isn't the End of Device Path Node,
                // check its validation
                //
                if(
                    Node->DevPath.Type != ACPI_DEVICE_PATH ||
                    Node->DevPath.SubType != ACPI_ADR_DP ||
                    DevicePathNodeLength(&Node->DevPath) != sizeof(ACPI_ADR_DEVICE_PATH)
                )
                {
                    DbgPrint((DEBUG_INFO, "[%a]: %a:%d The device matched our Soft GPU, but for some reason was not valid: %r\n", gEfiCallerBaseName, __FILE__, __LINE__, Status));
                    Status = EFI_UNSUPPORTED;
                }
            }
        }
    }
    else
    {
        DbgPrint((DEBUG_INFO, "[%a]: %a:%d Device is unsupported.\n", gEfiCallerBaseName, __FILE__, __LINE__));
        Status = EFI_UNSUPPORTED;
    }
    
Done:
    gBS->CloseProtocol(
        Controller,
        &gEfiPciIoProtocolGuid,
        This->DriverBindingHandle,
        Controller
    );

    if(!EFI_ERROR(Status)) 
    {
        DbgPrint((DEBUG_INFO, "[%a]: Device is supported.\n", gEfiCallerBaseName));
    }

    return Status;
}

EFI_STATUS
EFIAPI
SoftGpuGopDriverBindingStart(
    IN EFI_DRIVER_BINDING_PROTOCOL  *This,
    IN EFI_HANDLE                   Controller,
    IN EFI_DEVICE_PATH_PROTOCOL     *RemainingDevicePath
)
{
    DbgPrint((DEBUG_INFO, "[%a]: %a:%d Starting SoftGpu GOP Driver.\n", gEfiCallerBaseName, __FILE__, __LINE__));

    BOOLEAN PciAttributesSaved = FALSE;

    EFI_STATUS Status = EFI_SUCCESS;
    ACPI_ADR_DEVICE_PATH AcpiDeviceNode;
    EFI_PCI_IO_PROTOCOL* PciIo;
    PCI_TYPE00 Pci;
    EFI_DEVICE_PATH_PROTOCOL* ParentDevicePath;

    SOFT_GPU_PRIVATE_DATA* Private = AllocateZeroPool(sizeof(SOFT_GPU_PRIVATE_DATA));

    if(!Private) 
    {
        DbgPrint((DEBUG_INFO, "[%a]: %a:%d Failed to allocate private data.\n", gEfiCallerBaseName, __FILE__, __LINE__));
        Status = EFI_OUT_OF_RESOURCES;
        goto FreeMemory;
    }

    Private->Signature = SOFT_GPU_PRIVATE_DATA_SIGNATURE;
    Private->Handle = NULL;

    //
    // Open the PCI I/O Protocol
    //
    Status = gBS->OpenProtocol(
        Controller,
        &gEfiPciIoProtocolGuid,
        (VOID**) &PciIo,
        This->DriverBindingHandle,
        Controller,
        EFI_OPEN_PROTOCOL_BY_DRIVER
    );

    // if(Status == EFI_ALREADY_STARTED)
    // {
    //     DbgPrint((DEBUG_INFO, "[%a]: %a:%d PciIo protocol already started. Apparently this is a good thing.\n", gEfiCallerBaseName, __FILE__, __LINE__));
    //     Status = EFI_SUCCESS;
    // }

    // ASSERT_EFI_ERROR(Status);

    if(EFI_ERROR(Status))
    {
        DbgPrint((DEBUG_INFO, "[%a]: %a:%d Error opening PciIo protocol: %r.\n", gEfiCallerBaseName, __FILE__, __LINE__, Status));
        goto FreeMemory;
    }
    
    Private->PciIo = PciIo;

    //
    // Read the PCI Configuration Header from the PCI Device again to figure out the model.
    //
    Status = PciIo->Pci.Read(
        PciIo, 
        EfiPciIoWidthUint32, 
        0, 
        sizeof(Pci) / sizeof(UINT32), 
        &Pci
    );

    if(EFI_ERROR(Status))
    {
        DbgPrint((DEBUG_INFO, "[%a]: %a:%d Error reading PCI Config: %r\n", gEfiCallerBaseName, __FILE__, __LINE__, Status));
        goto CloseProtocols;
    }

    Private->DeviceType = Pci.Hdr.DeviceId;

    //
    // Save original PCI attributes
    //
    Status = PciIo->Attributes(
        PciIo,
        EfiPciIoAttributeOperationGet,
        0,
        &Private->OriginalPciAttributes
    );

    if(EFI_ERROR(Status))
    {
        DbgPrint((DEBUG_INFO, "[%a]: %a:%d Error saving original PCI attributes: %r\n", gEfiCallerBaseName, __FILE__, __LINE__, Status));
        goto CloseProtocols;
    }

    PciAttributesSaved = TRUE;

    //
    // Enable the device
    //
    Status = PciIo->Attributes(
        PciIo,
        EfiPciIoAttributeOperationEnable,
        EFI_PCI_DEVICE_ENABLE,
        NULL
    );
    
    if(EFI_ERROR(Status))
    {
        DbgPrint((DEBUG_INFO, "[%a]: %a:%d Error enabling the PCI device: %r\n", gEfiCallerBaseName, __FILE__, __LINE__, Status));
        goto RestorePciAttributes;
    }

    //    
    // Get ParentDevicePath
    //
    Status = gBS->HandleProtocol(
        Controller,
        &gEfiDevicePathProtocolGuid,
        (VOID**) &ParentDevicePath
    );

    if(EFI_ERROR (Status))
    {
        DbgPrint((DEBUG_INFO, "[%a]: %a:%d Error getting the Parent Device Path: %r\n", gEfiCallerBaseName, __FILE__, __LINE__, Status));
        goto RestorePciAttributes;
    }

    //
    // Set Gop Device Path
    //
    if(!RemainingDevicePath)
    {
        ZeroMem(&AcpiDeviceNode, sizeof(ACPI_ADR_DEVICE_PATH));
        AcpiDeviceNode.Header.Type = ACPI_DEVICE_PATH;
        AcpiDeviceNode.Header.SubType = ACPI_ADR_DP;
        AcpiDeviceNode.ADR = ACPI_DISPLAY_ADR(1, 0, 0, 1, 0, ACPI_ADR_DISPLAY_TYPE_VGA, 0, 0);
        SetDevicePathNodeLength(&AcpiDeviceNode.Header, sizeof(ACPI_ADR_DEVICE_PATH));

        Private->DevicePath = AppendDevicePathNode(
            ParentDevicePath,
            (EFI_DEVICE_PATH_PROTOCOL *) &AcpiDeviceNode
        );
    }
    else if(!IsDevicePathEnd(RemainingDevicePath))
    {
        //
        // If RemainingDevicePath isn't the End of Device Path Node,
        // only scan the specified device by RemainingDevicePath
        //
        Private->DevicePath = AppendDevicePathNode(ParentDevicePath, RemainingDevicePath);
    }
    else
    {
        //
        // If RemainingDevicePath is the End of Device Path Node,
        // don't create child device and return EFI_SUCCESS
        //
        Private->DevicePath = NULL;
    }

    if(Private->DevicePath)
    {
        Private->Handle = NULL;
        Status = gBS->InstallMultipleProtocolInterfaces(
            &Private->Handle,
            &gEfiDevicePathProtocolGuid,
            Private->DevicePath,
            NULL
        );
    }

    const volatile UINT8* SoftGpuBar0 = (volatile UINT8*) (UINTN) (Pci.Device.Bar[0] & ~0x7);

    Private->BarIndexFrameBuffer = 1;
    const UINT32 VRAMSizeLow = *(volatile UINT32*) (SoftGpuBar0 + 0x0014);
    const UINT32 VRAMSizeHigh = *(volatile UINT32*) (SoftGpuBar0 + 0x0018);
    Private->VRAMSize = (((UINT64) VRAMSizeHigh) << 32) | (UINT64) VRAMSizeLow;

    DbgPrint((DEBUG_INFO, "[%a]: %a:%d VRAM Size: 0x%llX\n", gEfiCallerBaseName, __FILE__, __LINE__, Private->VRAMSize));

    Status = SoftGpuVideoModeSetup(Private);
    if(EFI_ERROR(Status))
    {
        goto RestorePciAttributes;
    }

    if(!Private->DevicePath)
    {
        //
        // If RemainingDevicePath is the End of Device Path Node,
        // don't create child device and return EFI_SUCCESS
        //
        Status = EFI_SUCCESS;
    }
    else
    {
        // const UINT32 DisplayWidth = *(volatile UINT32*) (SoftGpuBar0 + 0x2000);
        // const UINT32 DisplayHeight = *(volatile UINT32*) (SoftGpuBar0 + 0x2004);

        // DbgPrint((DEBUG_INFO, "[%a]: %a:%d Display: %dx%d\n", gEfiCallerBaseName, __FILE__, __LINE__, DisplayWidth, DisplayHeight));

        Status = SoftGpuGopConstructor(Private);

        ASSERT_EFI_ERROR(Status);

        Status = gBS->InstallMultipleProtocolInterfaces(
            &Private->Handle,
            &gEfiGraphicsOutputProtocolGuid,
            &Private->GraphicsOutput,
            &gEfiEdidDiscoveredProtocolGuid,
            &Private->EdidDiscovered,
            &gEfiEdidActiveProtocolGuid,
            &Private->EdidActive,
            NULL
        );
    }

    mDriverStarted = TRUE;

    // if(!EFI_ERROR(Status))
    // {
    //     Status = gBS->OpenProtocol(
    //         Controller,
    //         &gEfiPciIoProtocolGuid,
    //         (VOID**) &Private->PciIo,
    //         This->DriverBindingHandle,
    //         Private->GraphicsOutputHandle,
    //         EFI_OPEN_PROTOCOL_BY_CHILD_CONTROLLER
    //     );

    //     if(!EFI_ERROR(Status))
    //     {
    //         mDriverStarted = TRUE;
    //     }
    //     else
    //     {
    //         gBS->UninstallMultipleProtocolInterfaces(
    //             Private->GraphicsOutputHandle,
    //             &gEfiGraphicsOutputProtocolGuid,
    //             &Private->GraphicsOutput,
    //             &gEfiDevicePathProtocolGuid,
    //             Private->DevicePath,
    //             NULL
    //         );
    //     }
    // }

RestorePciAttributes:
    if(EFI_ERROR(Status))
    {
        if(PciAttributesSaved)
        {
            //
            // Restore original PCI attributes
            //
            PciIo->Attributes(
                PciIo,
                EfiPciIoAttributeOperationSet,
                Private->OriginalPciAttributes,
                NULL
            );
        }
    }

CloseProtocols:
    if(EFI_ERROR(Status))
    {        
        if(PciAttributesSaved)
        {
            Status = PciIo->Attributes(
                PciIo,
                EfiPciIoAttributeOperationEnable,
                Private->OriginalPciAttributes,
                NULL
            );
        }

        //
        // Close the PCI I/O Protocol
        //
        gBS->CloseProtocol(
            Controller,
            &gEfiPciIoProtocolGuid,
            This->DriverBindingHandle,
            Controller
        );

        Private->PciIo = NULL;
    }

FreeMemory:
    if(EFI_ERROR(Status))
    {
        if(Private)
        {
            if(Private->DevicePath)
            {
                FreePool(Private->DevicePath);
            }

            FreePool(Private);
        }
    }

    return Status;
}

EFI_STATUS
EFIAPI
SoftGpuGopDriverBindingStop(
    IN EFI_DRIVER_BINDING_PROTOCOL  *This,
    IN EFI_HANDLE                   Controller,
    IN UINTN                        NumberOfChildren,
    IN EFI_HANDLE                   *ChildHandleBuffer
)
{
    DbgPrint((DEBUG_INFO, "[%a]: %a:%d Stopping SoftGpu GOP Driver.\n", gEfiCallerBaseName, __FILE__, __LINE__));

    EFI_GRAPHICS_OUTPUT_PROTOCOL* GraphicsOutput;

    EFI_STATUS Status = gBS->OpenProtocol(
        Controller,
        &gEfiGraphicsOutputProtocolGuid,
        (VOID**) &GraphicsOutput,
        This->DriverBindingHandle,
        Controller,
        EFI_OPEN_PROTOCOL_GET_PROTOCOL
    );

    if(EFI_ERROR(Status))
    {
        DbgPrint((DEBUG_INFO, "[%a]: %a:%d Error getting the Graphics Output Protocol: %r\n", gEfiCallerBaseName, __FILE__, __LINE__, Status));
        return Status;
    }

    SOFT_GPU_PRIVATE_DATA* Private = SOFT_GPU_PRIVATE_FROM_THIS(GraphicsOutput);

    Status = SoftGpuGopDestructor(Private);
 
    if(EFI_ERROR(Status))
    {
        DbgPrint((DEBUG_INFO, "[%a]: %a:%d Error destructing the Soft GPU driver private data: %r\n", gEfiCallerBaseName, __FILE__, __LINE__, Status));
        // return Status;
    }

    Status = gBS->UninstallMultipleProtocolInterfaces(
        Private->Handle,
        &gEfiGraphicsOutputProtocolGuid,
        &Private->GraphicsOutput,
        NULL
    );

    if(EFI_ERROR(Status))
    {
        DbgPrint((DEBUG_INFO, "[%a]: %a:%d Error uninstalling Graphics Output Protocol: %r\n", gEfiCallerBaseName, __FILE__, __LINE__, Status));
        return Status;
    }

    Status = Private->PciIo->Attributes(
        Private->PciIo,
        EfiPciIoAttributeOperationSet,
        Private->OriginalPciAttributes,
        NULL
    );

    if(EFI_ERROR(Status))
    {
        DbgPrint((DEBUG_INFO, "[%a]: %a:%d Error restoring PCI Attributes: %r\n", gEfiCallerBaseName, __FILE__, __LINE__, Status));
        // return Status;
    }

    gBS->CloseProtocol(
        Controller,
        &gEfiPciIoProtocolGuid,
        This->DriverBindingHandle,
        Controller
    );

    gBS->FreePool(Private);

    return EFI_SUCCESS;
}

/**
  The user Entry Point for Application. The user code starts with this function
  as the real entry point for the application.

  @param[in] ImageHandle    The firmware allocated handle for the EFI image.
  @param[in] SystemTable    A pointer to the EFI System Table.

  @retval EFI_SUCCESS       The entry point is executed successfully.
  @retval other             Some error occurs when executing this entry point.

**/
EFI_STATUS
EFIAPI
InitializeSoftGpuGop(
    IN EFI_HANDLE        ImageHandle,
    IN EFI_SYSTEM_TABLE  *SystemTable
)
{
    DEBUG((DEBUG_INFO, "Initializing SoftGpu GOP Driver.\n"));

    DbgPrint((DEBUG_INFO, "[%a]: %a:%d Initializing SoftGpu GOP Driver.\n", gEfiCallerBaseName, __FILE__, __LINE__));
    
    EFI_STATUS Status = EfiLibInstallDriverBindingComponentName2(
        ImageHandle,
        SystemTable,
        &gSoftGpuGopDriverBinding,
        ImageHandle,
        &gSoftGpuGopComponentName,
        &gSoftGpuGopComponentName2
    );
    
    DbgPrint((DEBUG_INFO, "[%a]: SoftGpu GOP Driver - Installed Driver Binding & Component Name 2.\n", gEfiCallerBaseName));
    DbgPrint((DEBUG_INFO, "[%a]: SoftGpu GOP Driver - Status: %r.\n", gEfiCallerBaseName, Status));

    ASSERT_EFI_ERROR(Status);

    return Status;
}

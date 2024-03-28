#include "SoftGpuGop.h"

#define EDID_BLOCK_SIZE (128)

EFI_STATUS
SoftGpuVideoModeSetup(
    SOFT_GPU_PRIVATE_DATA* Private
)
{
    Private->EdidDiscovered.Edid = NULL;
    Private->EdidDiscovered.SizeOfEdid = 0;
    Private->EdidActive.Edid = NULL;
    Private->EdidActive.SizeOfEdid = 0;

    // BOOLEAN EdidFound = FALSE;
    BOOLEAN EdidOverrideFound = FALSE;
    
    UINT32 EdidAttributes = 0xFF;
    UINTN EdidOverrideDataSize = 0;
    UINT8* EdidOverrideDataBlock = NULL;

    EFI_EDID_OVERRIDE_PROTOCOL* EdidOverride;

    //
    // Find EDID Override protocol first, this protocol is installed by platform if needed.
    //
    EFI_STATUS Status = gBS->LocateProtocol(
        &gEfiEdidOverrideProtocolGuid,
        NULL,
        (VOID**) &EdidOverride
    );

    // This code is sourced from VirtualBox Edid, and has clearly never been tested due to a bug I fixed.
    if(!EFI_ERROR(Status))
    {
        //
        // Allocate double size of VESA_BIOS_EXTENSIONS_EDID_BLOCK_SIZE to avoid overflow
        //
        EdidOverrideDataBlock = AllocatePool(EDID_BLOCK_SIZE * 2);
        if(NULL == EdidOverrideDataBlock)
        {
            Status = EFI_OUT_OF_RESOURCES;
            goto Done;
        }

        Status = EdidOverride->GetEdid(
            EdidOverride,
            Private->Handle,
            &EdidAttributes,
            &EdidOverrideDataSize,
            (UINT8**) &EdidOverrideDataBlock
        );

        if(
            !EFI_ERROR(Status)  &&
            EdidAttributes == 0 &&
            EdidOverrideDataSize != 0
        )
        {
            //
            // Succeeded at getting the EDID Override Data
            //
            EdidOverrideFound = TRUE;
        }
    }

    if(!EdidOverrideFound || EdidAttributes == EFI_EDID_OVERRIDE_DONT_OVERRIDE)
    {

    }

Done:

    return EFI_SUCCESS;
}

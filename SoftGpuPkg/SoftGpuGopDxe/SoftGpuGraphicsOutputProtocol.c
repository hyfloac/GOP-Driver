#include "SoftGpuGop.h"
#include <IndustryStandard/Acpi.h>

EFI_STATUS
EFIAPI
SoftGpuGopQueryMode(
    IN  EFI_GRAPHICS_OUTPUT_PROTOCOL* This,
    IN  UINT32 ModeNumber,
    OUT UINTN* SizeOfInfo,
    OUT EFI_GRAPHICS_OUTPUT_MODE_INFORMATION** Info
)
{
    SOFT_GPU_PRIVATE_DATA* Private = SOFT_GPU_PRIVATE_FROM_THIS(This);

    if(Private->NeedsStart)
    {
        return EFI_NOT_STARTED;
    }

    if(!Info || !SizeOfInfo || ModeNumber >= This->Mode->MaxMode)
    {
        return EFI_INVALID_PARAMETER;
    }

    EFI_GRAPHICS_OUTPUT_MODE_INFORMATION* AllocatedInfo = AllocatePool(sizeof(EFI_GRAPHICS_OUTPUT_MODE_INFORMATION));

    if(!AllocatedInfo)
    {
        return EFI_OUT_OF_RESOURCES;
    }

    AllocatedInfo->Version = 0;
    AllocatedInfo->HorizontalResolution = Private->ModeData[ModeNumber].HorizontalResolution;
    AllocatedInfo->VerticalResolution   = Private->ModeData[ModeNumber].VerticalResolution;
    AllocatedInfo->PixelFormat = PixelRedGreenBlueReserved8BitPerColor;
    (void) ZeroMem(&AllocatedInfo->PixelInformation, sizeof(AllocatedInfo->PixelInformation));
    AllocatedInfo->PixelsPerScanLine = AllocatedInfo->HorizontalResolution;

    *Info = AllocatedInfo;
    *SizeOfInfo = sizeof(EFI_GRAPHICS_OUTPUT_MODE_INFORMATION);

    return EFI_SUCCESS;
}

EFI_STATUS
SoftGpuGopConstructor(
    SOFT_GPU_PRIVATE_DATA* Private
)
{
    DbgPrint((DEBUG_INFO, "[%a]: %a:%d Constructing Soft GPU Private Data\n", gEfiCallerBaseName, __FILE__, __LINE__));

    EFI_GRAPHICS_OUTPUT_PROTOCOL *GraphicsOutput = &Private->GraphicsOutput;
    GraphicsOutput->QueryMode = SoftGpuGopQueryMode;
    GraphicsOutput->SetMode = NULL;
    GraphicsOutput->Blt = NULL;

    EFI_STATUS Status = gBS->AllocatePool(
        EfiBootServicesData,
        sizeof(EFI_GRAPHICS_OUTPUT_PROTOCOL_MODE),
        (VOID**) &GraphicsOutput->Mode
    );

    if(EFI_ERROR(Status))
    {
        DbgPrint((DEBUG_INFO, "[%a]: %a:%d Failed to allocate GOP Mode.\n", gEfiCallerBaseName, __FILE__, __LINE__));
        return Status;
    }

    Status = gBS->AllocatePool(
        EfiBootServicesData,
        sizeof(EFI_GRAPHICS_OUTPUT_MODE_INFORMATION),
        (VOID**) &GraphicsOutput->Mode->Info
    );
    
    if(EFI_ERROR(Status))
    {
        DbgPrint((DEBUG_INFO, "[%a]: %a:%d Failed to allocate GOP Mode Info.\n", gEfiCallerBaseName, __FILE__, __LINE__));
        return Status;
    }

    GraphicsOutput->Mode->MaxMode = (UINT32) Private->MaxMode;
    GraphicsOutput->Mode->Mode = GRAPHICS_OUTPUT_INVALID_MODE_NUMBER;
    Private->NeedsStart = TRUE;

    return EFI_SUCCESS;
}

EFI_STATUS
SoftGpuGopDestructor(
    SOFT_GPU_PRIVATE_DATA* Private
)
{
    if(!Private)
    {
        return EFI_SUCCESS;
    }

    if(Private->GraphicsOutput.Mode)
    {
        if(Private->GraphicsOutput.Mode->Info)
        {
            gBS->FreePool(Private->GraphicsOutput.Mode->Info);
        }

        gBS->FreePool(Private->GraphicsOutput.Mode);
    }

    return EFI_SUCCESS;
}

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
    DbgPrint((DEBUG_INFO, "[%a]: %a:%d Querying VGA Mode %d\n", gEfiCallerBaseName, __FILE__, __LINE__, ModeNumber));

    SOFT_GPU_PRIVATE_DATA* Private = SOFT_GPU_PRIVATE_FROM_THIS(This);

    // if(Private->NeedsStart)
    // {
    //     DbgPrint((DEBUG_INFO, "[%a]: %a:%d GPU not started\n", gEfiCallerBaseName, __FILE__, __LINE__));
    //     return EFI_NOT_STARTED;
    // }

    if(!Info || !SizeOfInfo || ModeNumber >= This->Mode->MaxMode)
    {
        DbgPrint((DEBUG_INFO, "[%a]: %a:%d Invalid Parameter\n", gEfiCallerBaseName, __FILE__, __LINE__));
        return EFI_INVALID_PARAMETER;
    }

    EFI_GRAPHICS_OUTPUT_MODE_INFORMATION* AllocatedInfo = AllocatePool(sizeof(EFI_GRAPHICS_OUTPUT_MODE_INFORMATION));

    if(!AllocatedInfo)
    {
        DbgPrint((DEBUG_INFO, "[%a]: %a:%d Failed to allocate mode info\n", gEfiCallerBaseName, __FILE__, __LINE__));
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

static UINT16 BASE_REGISTER_DI            = 0x2000;
static UINT16 SIZE_REGISTER_DI            = 6 * 0x4;
static UINT16 OFFSET_REGISTER_DI_WIDTH    = 0x00;
static UINT16 OFFSET_REGISTER_DI_HEIGHT   = 0x04;
static UINT16 OFFSET_REGISTER_DI_BPP      = 0x08;
static UINT16 OFFSET_REGISTER_DI_ENABLE   = 0x0C;
static UINT16 OFFSET_REGISTER_DI_REFRESH_RATE_NUMERATOR   = 0x10;
static UINT16 OFFSET_REGISTER_DI_REFRESH_RATE_DENOMINATOR = 0x14;

EFI_STATUS
EFIAPI
SoftGpuGopSetMode(
    IN EFI_GRAPHICS_OUTPUT_PROTOCOL* This,
    IN UINT32 ModeNumber
)
{
    DbgPrint((DEBUG_INFO, "[%a]: %a:%d Setting VGA Mode %d\n", gEfiCallerBaseName, __FILE__, __LINE__, ModeNumber));

    SOFT_GPU_PRIVATE_DATA* Private = SOFT_GPU_PRIVATE_FROM_THIS(This);

    if(ModeNumber > This->Mode->MaxMode)
    {
        return EFI_UNSUPPORTED;
    }

    SOFT_GPU_VGA_MODE_DATA* ModeData = &Private->ModeData[ModeNumber];

    UINT32 Value = 0;

    // Init graphics mode
    EFI_STATUS Status = Private->PciIo->Mem.Write(
        Private->PciIo,
        EfiPciIoWidthUint32,
        0,
        BASE_REGISTER_DI + SIZE_REGISTER_DI * 0 + OFFSET_REGISTER_DI_ENABLE,
        1,
        &Value
    );

    if(EFI_ERROR(Status))
    {
        DbgPrint((DEBUG_INFO, "[%a]: %a:%d Failed to disable display 0. %r\n", gEfiCallerBaseName, __FILE__, __LINE__, Status));
        return EFI_DEVICE_ERROR;
    }

    Value = ModeData->HorizontalResolution;

    Status = Private->PciIo->Mem.Write(
        Private->PciIo,
        EfiPciIoWidthUint32,
        0,
        BASE_REGISTER_DI + SIZE_REGISTER_DI * 0 + OFFSET_REGISTER_DI_WIDTH,
        1,
        &Value
    );

    if(EFI_ERROR(Status))
    {
        DbgPrint((DEBUG_INFO, "[%a]: %a:%d Failed to set width %d for display 0. %r\n", gEfiCallerBaseName, __FILE__, __LINE__, Value, Status));
        return EFI_DEVICE_ERROR;
    }

    Value = ModeData->VerticalResolution;

    Status = Private->PciIo->Mem.Write(
        Private->PciIo,
        EfiPciIoWidthUint32,
        0,
        BASE_REGISTER_DI + SIZE_REGISTER_DI * 0 + OFFSET_REGISTER_DI_HEIGHT,
        1,
        &Value
    );

    if(EFI_ERROR(Status))
    {
        DbgPrint((DEBUG_INFO, "[%a]: %a:%d Failed to set height %d for display 0. %r\n", gEfiCallerBaseName, __FILE__, __LINE__, Value, Status));
        return EFI_DEVICE_ERROR;
    }

    Value = ModeData->RefreshRate;

    Status = Private->PciIo->Mem.Write(
        Private->PciIo,
        EfiPciIoWidthUint32,
        0,
        BASE_REGISTER_DI + SIZE_REGISTER_DI * 0 + OFFSET_REGISTER_DI_REFRESH_RATE_NUMERATOR,
        1,
        &Value
    );

    if(EFI_ERROR(Status))
    {
        DbgPrint((DEBUG_INFO, "[%a]: %a:%d Failed to set Refresh Rate Numerator %d for display 0. %r\n", gEfiCallerBaseName, __FILE__, __LINE__, Value, Status));
        return EFI_DEVICE_ERROR;
    }

    Value = 1;

    Status = Private->PciIo->Mem.Write(
        Private->PciIo,
        EfiPciIoWidthUint32,
        0,
        BASE_REGISTER_DI + SIZE_REGISTER_DI * 0 + OFFSET_REGISTER_DI_REFRESH_RATE_DENOMINATOR,
        1,
        &Value
    );

    if(EFI_ERROR(Status))
    {
        DbgPrint((DEBUG_INFO, "[%a]: %a:%d Failed to set Refresh Rate Denominator %d for display 0. %r\n", gEfiCallerBaseName, __FILE__, __LINE__, Value, Status));
        return EFI_DEVICE_ERROR;
    }

    Value = 1;
    
    Status = Private->PciIo->Mem.Write(
        Private->PciIo,
        EfiPciIoWidthUint32,
        0,
        BASE_REGISTER_DI + SIZE_REGISTER_DI * 0 + OFFSET_REGISTER_DI_ENABLE,
        1,
        &Value
    );

    if(EFI_ERROR(Status))
    {
        DbgPrint((DEBUG_INFO, "[%a]: %a:%d Failed to enable display 0. %r\n", gEfiCallerBaseName, __FILE__, __LINE__, Status));
        return EFI_DEVICE_ERROR;
    }

    This->Mode->Mode = ModeNumber;
    This->Mode->Info->Version = 0;
    This->Mode->Info->HorizontalResolution = ModeData->HorizontalResolution;
    This->Mode->Info->VerticalResolution = ModeData->VerticalResolution;
    This->Mode->Info->PixelFormat = PixelRedGreenBlueReserved8BitPerColor;
    (void) ZeroMem(&This->Mode->Info->PixelInformation, sizeof(This->Mode->Info->PixelInformation));
    This->Mode->Info->PixelsPerScanLine = ModeData->HorizontalResolution;
    This->Mode->SizeOfInfo = sizeof(EFI_GRAPHICS_OUTPUT_MODE_INFORMATION);

    EFI_ACPI_ADDRESS_SPACE_DESCRIPTOR* FrameBufferDesc;
    Status = Private->PciIo->GetBarAttributes(
        Private->PciIo,
        Private->BarIndexFrameBuffer,
        NULL,
        (VOID**) &FrameBufferDesc
    );

    if(EFI_ERROR(Status))
    {
        DbgPrint((DEBUG_INFO, "[%a]: %a:%d Error Loading Framebuffer: %r\n", gEfiCallerBaseName, __FILE__, __LINE__, Status));
        return EFI_OUT_OF_RESOURCES;
    }

    DbgPrint((DEBUG_INFO, "[%a]: %a:%d FrameBufferBase: 0x%0llX, Bar: %d\n", gEfiCallerBaseName, __FILE__, __LINE__, FrameBufferDesc->AddrRangeMin, Private->BarIndexFrameBuffer));

    This->Mode->FrameBufferBase = FrameBufferDesc->AddrRangeMin;
    This->Mode->FrameBufferSize = ModeData->HorizontalResolution * ModeData->VerticalResolution * sizeof(EFI_GRAPHICS_OUTPUT_BLT_PIXEL);

    Private->NeedsStart = FALSE;
    // Update current mode.
    Private->CurrentMode = ModeNumber;

    return EFI_SUCCESS;
}

/**
 * 
 * Routine Description:
 * 
 *   Graphics Output protocol instance to block transfer for CirrusLogic device
 * 
 * Arguments:
 * 
 *   This          - Pointer to Graphics Output protocol instance
 *   BltBuffer     - The data to transfer to screen
 *   BltOperation  - The operation to perform
 *   SourceX       - The X coordinate of the source for BltOperation
 *   SourceY       - The Y coordinate of the source for BltOperation
 *   DestinationX  - The X coordinate of the destination for BltOperation
 *   DestinationY  - The Y coordinate of the destination for BltOperation
 *   Width         - The width of a rectangle in the blt rectangle in pixels
 *   Height        - The height of a rectangle in the blt rectangle in pixels
 *   Delta         - Not used for EfiBltVideoFill and EfiBltVideoToVideo operation.
 *                   If a Delta of 0 is used, the entire BltBuffer will be operated on.
 *                   If a subrectangle of the BltBuffer is used, then Delta represents
 *                   the number of bytes in a row of the BltBuffer.
 * 
 * Returns:
 * 
 *   EFI_INVALID_PARAMETER - Invalid parameter passed in
 *   EFI_SUCCESS - Blt operation success
 * 
 */
EFI_STATUS
EFIAPI
SoftGpuGopOutputBlt(
    IN EFI_GRAPHICS_OUTPUT_PROTOCOL* This,
    IN OPTIONAL EFI_GRAPHICS_OUTPUT_BLT_PIXEL* BltBuffer,
    IN EFI_GRAPHICS_OUTPUT_BLT_OPERATION BltOperation,
    IN UINTN SourceX,
    IN UINTN SourceY,
    IN UINTN DestinationX,
    IN UINTN DestinationY,
    IN UINTN Width,
    IN UINTN Height,
    IN UINTN Delta
)
{
    // DbgPrint((DEBUG_INFO, "[%a]: %a:%d Output Blit: %d, %dx%d %dx%d %dx%d Delta: %d\n", gEfiCallerBaseName, __FILE__, __LINE__, BltOperation, SourceX, SourceY, DestinationX, DestinationY, Width, Height, Delta));

    SOFT_GPU_PRIVATE_DATA* Private = SOFT_GPU_PRIVATE_FROM_THIS(This);

    const UINT32 CurrentMode = This->Mode->Mode;

    if(CurrentMode > Private->MaxMode)
    {
        return EFI_INVALID_PARAMETER;
    }

    const UINTN ScreenWidth = Private->ModeData[CurrentMode].HorizontalResolution;
    const UINTN ScreenHeight = Private->ModeData[CurrentMode].VerticalResolution;

    // DbgPrint((DEBUG_INFO, "[%a]: %a:%d Screen: %dx%d Mode: %d\n", gEfiCallerBaseName, __FILE__, __LINE__, ScreenWidth, ScreenHeight, CurrentMode));

    if(BltOperation < 0 || BltOperation >= EfiGraphicsOutputBltOperationMax)
    {
        return EFI_INVALID_PARAMETER;
    }

    if(Width == 0 || Height == 0)
    {
        return EFI_INVALID_PARAMETER;
    }
    
    //
    // If Delta is zero, then the entire BltBuffer is being used, so Delta
    // is the number of bytes in each row of BltBuffer. Since BltBuffer is Width pixels size,
    // the number of bytes in each row can be computed.
    //
    if(Delta == 0)
    {
        Delta = Width * sizeof(EFI_GRAPHICS_OUTPUT_BLT_PIXEL);
    }

    // Code below assumes a Delta value in pixels, not bytes
    Delta /= sizeof(EFI_GRAPHICS_OUTPUT_BLT_PIXEL);

    //
    // Make sure the SourceX, SourceY, DestinationX, DestinationY, Width, and Height parameters
    // are valid for the operation and the current screen geometry.
    //
    if(BltOperation == EfiBltVideoToBltBuffer || BltOperation == EfiBltVideoToVideo)
    {
        if(SourceY + Height > ScreenHeight)
        {
            return EFI_INVALID_PARAMETER;
        }

        if(SourceX + Width > ScreenWidth)
        {
            return EFI_INVALID_PARAMETER;
        }
    }

    if(BltOperation == EfiBltBufferToVideo || BltOperation == EfiBltVideoToVideo || BltOperation == EfiBltVideoFill)
    {
        if(DestinationY + Height > ScreenHeight)
        {
            return EFI_INVALID_PARAMETER;
        }

        if(DestinationX + Width > ScreenWidth)
        {
            return EFI_INVALID_PARAMETER;
        }
    }
    
    EFI_STATUS Status;

    //
    // We have to raise to TPL Notify, so we make an atomic write the frame buffer.
    // We would not want a timer based event (Cursor, ...) to come in while we are
    // doing this operation.
    //
    EFI_TPL OriginalTPL = gBS->RaiseTPL(TPL_NOTIFY);

    switch(BltOperation)
    {
        case EfiBltVideoToBltBuffer:
            //
            // Video to BltBuffer: Source is Video, destination is BltBuffer
            //
            for(UINTN SrcY = SourceY, DstY = DestinationY; DstY < (Height + DestinationY) && BltBuffer; ++SrcY, ++DstY)
            {
                /// @todo assumes that color depth is 32 (*4, EfiPciIoWidthUint32) and format matches EFI_GRAPHICS_OUTPUT_BLT_PIXEL
                Status = Private->PciIo->Mem.Read(
                    Private->PciIo,
                    EfiPciIoWidthUint32,
                    Private->BarIndexFrameBuffer,
                    ((SrcY * ScreenWidth) + SourceX) * 4,
                    Width,
                    BltBuffer + (DstY * Delta) + DestinationX
                );
                ASSERT_EFI_ERROR((Status));
            }
            break;
        case EfiBltBufferToVideo:
            //
            // BltBuffer to Video: Source is BltBuffer, destination is Video
            //
            for(UINTN SrcY = SourceY, DstY = DestinationY; SrcY < (Height + SourceY); ++SrcY, ++DstY)
            {
                /// @todo assumes that color depth is 32 (*4, EfiPciIoWidthUint32) and format matches EFI_GRAPHICS_OUTPUT_BLT_PIXEL
                Status = Private->PciIo->Mem.Write(
                    Private->PciIo,
                    EfiPciIoWidthUint32,
                    Private->BarIndexFrameBuffer,
                    ((DstY * ScreenWidth) + DestinationX) * 4,
                    Width,
                    BltBuffer + (SrcY * Delta) + SourceX
                );
                ASSERT_EFI_ERROR((Status));
            }
            break;
            
        case EfiBltVideoToVideo:
            //
            // Video to Video: Source is Video, destination is Video
            //
            if(DestinationY <= SourceY)
            {
                // forward copy
                for(UINTN SrcY = SourceY, DstY = DestinationY; SrcY < (Height + SourceY); ++SrcY, ++DstY)
                {
                    /// @todo assumes that color depth is 32 (*4, EfiPciIoWidthUint32) and format matches EFI_GRAPHICS_OUTPUT_BLT_PIXEL
                    Status = Private->PciIo->CopyMem(
                        Private->PciIo,
                        EfiPciIoWidthUint32,
                        Private->BarIndexFrameBuffer,
                        ((DstY * ScreenWidth) + DestinationX) * 4,
                        Private->BarIndexFrameBuffer,
                        ((SrcY * ScreenWidth) + SourceX) * 4,
                        Width
                    );
                    ASSERT_EFI_ERROR((Status));
                }
            }
            else
            {
                // reverse copy
                for(UINTN SrcY = SourceY + Height - 1, DstY = DestinationY + Height - 1; SrcY >= SourceY && SrcY <= SourceY + Height - 1; --SrcY, --DstY)
                {
                    /// @todo assumes that color depth is 32 (*4, EfiPciIoWidthUint32) and format matches EFI_GRAPHICS_OUTPUT_BLT_PIXEL
                    Status = Private->PciIo->CopyMem(
                        Private->PciIo,
                        EfiPciIoWidthUint32,
                        Private->BarIndexFrameBuffer,
                        ((DstY * ScreenWidth) + DestinationX) * 4,
                        Private->BarIndexFrameBuffer,
                        ((SrcY * ScreenWidth) + SourceX) * 4,
                        Width
                    );
                    ASSERT_EFI_ERROR((Status));
                }
            }
            break;
            
        case EfiBltVideoFill:
            //
            // Video Fill: Source is BltBuffer, destination is Video
            //
            if(DestinationX == 0 && Width == ScreenWidth)
            {
                /// @todo assumes that color depth is 32 (*4, EfiPciIoWidthFillUint32) and format matches EFI_GRAPHICS_OUTPUT_BLT_PIXEL
                Status = Private->PciIo->Mem.Write(
                    Private->PciIo,
                    EfiPciIoWidthFillUint32,
                    Private->BarIndexFrameBuffer,
                    DestinationY * ScreenWidth * 4,
                    (Width * Height),
                    BltBuffer
                );
                ASSERT_EFI_ERROR((Status));
            }
            else
            {
                for(UINTN SrcY = SourceY, DstY = DestinationY; SrcY < (Height + SourceY); ++SrcY, ++DstY)
                {
                    /// @todo assumes that color depth is 32 (*4, EfiPciIoWidthFillUint32) and format matches EFI_GRAPHICS_OUTPUT_BLT_PIXEL
                    Status = Private->PciIo->Mem.Write(
                        Private->PciIo,
                        EfiPciIoWidthFillUint32,
                        Private->BarIndexFrameBuffer,
                        ((DstY * ScreenWidth) + DestinationX) * 4,
                        Width,
                        BltBuffer
                    );
                    ASSERT_EFI_ERROR((Status));
                }
            }
            break;
        default: 
            ASSERT(FALSE);
            break;
    }
    
    gBS->RestoreTPL(OriginalTPL);

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
    GraphicsOutput->SetMode = SoftGpuGopSetMode;
    GraphicsOutput->Blt = SoftGpuGopOutputBlt;

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

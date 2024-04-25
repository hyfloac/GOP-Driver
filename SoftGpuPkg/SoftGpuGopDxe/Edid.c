#include "SoftGpuGop.h"

#define EDID_BLOCK_SIZE (128)

//
// EDID block
//
typedef struct {
    UINT8   Header[8];                        //EDID header "00 FF FF FF FF FF FF 00"
    UINT16  ManufactureName;                  //EISA 3-character ID
    UINT16  ProductCode;                      //Vendor assigned code
    UINT32  SerialNumber;                     //32-bit serial number
    UINT8   WeekOfManufacture;                //Week number
    UINT8   YearOfManufacture;                //Year
    UINT8   EdidVersion;                      //EDID Structure Version
    UINT8   EdidRevision;                     //EDID Structure Revision
    UINT8   VideoInputDefinition;
    UINT8   MaxHorizontalImageSize;           //cm
    UINT8   MaxVerticalImageSize;             //cm
    UINT8   DisplayTransferCharacteristic;
    UINT8   FeatureSupport;
    UINT8   RedGreenLowBits;                  //Rx1 Rx0 Ry1 Ry0 Gx1 Gx0 Gy1Gy0
    UINT8   BlueWhiteLowBits;                 //Bx1 Bx0 By1 By0 Wx1 Wx0 Wy1 Wy0
    UINT8   RedX;                             //Red-x Bits 9 - 2
    UINT8   RedY;                             //Red-y Bits 9 - 2
    UINT8   GreenX;                           //Green-x Bits 9 - 2
    UINT8   GreenY;                           //Green-y Bits 9 - 2
    UINT8   BlueX;                            //Blue-x Bits 9 - 2
    UINT8   BlueY;                            //Blue-y Bits 9 - 2
    UINT8   WhiteX;                           //White-x Bits 9 - 2
    UINT8   WhiteY;                           //White-x Bits 9 - 2
    UINT8   EstablishedTimings[3];
    UINT8   StandardTimingIdentification[16];
    UINT8   DetailedTimingDescriptions[72];
    UINT8   ExtensionFlag;                    //Number of (optional) 128-byte EDID extension blocks to follow
    UINT8   Checksum;
} EDID_BLOCK;

#define VBE_EDID_ESTABLISHED_TIMING_MAX_NUMBER 17

typedef struct {
    UINT16  HorizontalResolution;
    UINT16  VerticalResolution;
    UINT16  RefreshRate;
} EDID_TIMING;

typedef struct {
    UINT32  ValidNumber;
    UINT32  Key[VBE_EDID_ESTABLISHED_TIMING_MAX_NUMBER];
} VALID_EDID_TIMING;


//
// Standard timing defined by VESA EDID
//
EDID_TIMING mVbeEstablishedEdidTiming[] = 
{
  //
  // Established Timing I
  //
  {800, 600, 60},
  {800, 600, 56},
  {640, 480, 75},
  {640, 480, 72},
  {640, 480, 67},
  {640, 480, 60},
  {720, 400, 88},
  {720, 400, 70},
  //
  // Established Timing II
  //
  {1280, 1024, 75},
  {1024,  768, 75},
  {1024,  768, 70},
  {1024,  768, 60},
  {1024,  768, 87},
  {832,   624, 75},
  {800,   600, 75},
  {800,   600, 72},
  //
  // Established Timing III
  //
  {1152, 870, 75}
};


///
/// Table of supported video modes (sorted by increasing horizontal, then by
/// increasing vertical resolution)
///
SOFT_GPU_VGA_VIDEO_MODES gSoftGpuGopVideoModes[] =
{
  {  640,  480, 32, 60, NULL /* crtc */, NULL /* sequencer */, 0x01 }, // VGA 4:3
  {  800,  600, 32, 60, NULL /* crtc */, NULL /* sequencer */, 0x01 }, // SVGA 4:3
  { 1024,  768, 32, 60, NULL /* crtc */, NULL /* sequencer */, 0x01 }, // XGA 4:3
  { 1152,  864, 32, 60, NULL /* crtc */, NULL /* sequencer */, 0x01 }, // XGA+ 4:3
  { 1280,  720, 32, 60, NULL /* crtc */, NULL /* sequencer */, 0x01 }, // HD 16:9
  { 1280,  800, 32, 60, NULL /* crtc */, NULL /* sequencer */, 0x01 }, // WXGA 16:10
  { 1280, 1024, 32, 60, NULL /* crtc */, NULL /* sequencer */, 0x01 }, // SXGA 5:4
  { 1400, 1050, 32, 60, NULL /* crtc */, NULL /* sequencer */, 0x01 }, // SXGA+ 4:3
  { 1440,  900, 32, 60, NULL /* crtc */, NULL /* sequencer */, 0x01 }, // WXGA+ 16:10
  { 1600,  900, 32, 60, NULL /* crtc */, NULL /* sequencer */, 0x01 }, // HD+ 16:9
  { 1600, 1200, 32, 60, NULL /* crtc */, NULL /* sequencer */, 0x01 }, // UXGA 4:3
  { 1680, 1050, 32, 60, NULL /* crtc */, NULL /* sequencer */, 0x01 }, // WSXGA+ 16:10
  { 1920, 1080, 32, 60, NULL /* crtc */, NULL /* sequencer */, 0x01 }, // FHD 16:9
  { 1920, 1200, 32, 60, NULL /* crtc */, NULL /* sequencer */, 0x01 }, // WUXGA 16:10
  { 2048, 1080, 32, 60, NULL /* crtc */, NULL /* sequencer */, 0x01 }, // DCI_2K 19:10
  { 2160, 1440, 32, 60, NULL /* crtc */, NULL /* sequencer */, 0x01 }, // FHD+ 3:2
  { 2304, 1440, 32, 60, NULL /* crtc */, NULL /* sequencer */, 0x01 }, // unnamed 16:10
  { 2560, 1440, 32, 60, NULL /* crtc */, NULL /* sequencer */, 0x01 }, // QHD 16:9
  { 2560, 1600, 32, 60, NULL /* crtc */, NULL /* sequencer */, 0x01 }, // WQXGA 16:10
  { 2880, 1800, 32, 60, NULL /* crtc */, NULL /* sequencer */, 0x01 }, // QWXGA+ 16:10
  { 3200, 1800, 32, 60, NULL /* crtc */, NULL /* sequencer */, 0x01 }, // QHD+ 16:9
  { 3200, 2048, 32, 60, NULL /* crtc */, NULL /* sequencer */, 0x01 }, // WQSXGA 16:10
  { 3840, 2160, 32, 60, NULL /* crtc */, NULL /* sequencer */, 0x01 }, // 4K_UHD 16:9
  { 3840, 2400, 32, 60, NULL /* crtc */, NULL /* sequencer */, 0x01 }, // WQUXGA 16:10
  { 4096, 2160, 32, 60, NULL /* crtc */, NULL /* sequencer */, 0x01 }, // DCI_4K 19:10
  { 4096, 3072, 32, 60, NULL /* crtc */, NULL /* sequencer */, 0x01 }, // HXGA 4:3
  { 5120, 2880, 32, 60, NULL /* crtc */, NULL /* sequencer */, 0x01 }, // UHD+ 16:9
  { 5120, 3200, 32, 60, NULL /* crtc */, NULL /* sequencer */, 0x01 }, // WHXGA 16:10
  { 6400, 4096, 32, 60, NULL /* crtc */, NULL /* sequencer */, 0x01 }, // WHSXGA 16:10
  { 6400, 4800, 32, 60, NULL /* crtc */, NULL /* sequencer */, 0x01 }, // HUXGA 4:3
  { 7680, 4320, 32, 60, NULL /* crtc */, NULL /* sequencer */, 0x01 }, // 8K_UHD2 16:9
  { 0, }, // Custom video mode 0, do not delete, must be at the end!
  { 0, }, // Custom video mode 1, do not delete, must be at the end!
  { 0, }, // Custom video mode 2, do not delete, must be at the end!
  { 0, }, // Custom video mode 3, do not delete, must be at the end!
  { 0, }, // Custom video mode 4, do not delete, must be at the end!
  { 0, }, // Custom video mode 5, do not delete, must be at the end!
  { 0, }, // Custom video mode 6, do not delete, must be at the end!
  { 0, }, // Custom video mode 7, do not delete, must be at the end!
  { 0, }, // Custom video mode 8, do not delete, must be at the end!
  { 0, }, // Custom video mode 9, do not delete, must be at the end!
  { 0, }, // Custom video mode 10, do not delete, must be at the end!
  { 0, }, // Custom video mode 11, do not delete, must be at the end!
  { 0, }, // Custom video mode 12, do not delete, must be at the end!
  { 0, }, // Custom video mode 13, do not delete, must be at the end!
  { 0, }, // Custom video mode 14, do not delete, must be at the end!
  { 0, }  // Custom video mode 15, do not delete, must be at the end!
};

const UINT32 gSoftGpuGopVideoModeCount = sizeof(gSoftGpuGopVideoModes) / sizeof(gSoftGpuGopVideoModes[0]);

EFI_STATUS
ReadEdidData(
    SOFT_GPU_PRIVATE_DATA* Private,
    UINT8** EdidDataBlock,
    UINTN* EdidSize
)
{
    DbgPrint((DEBUG_INFO, "[%a]: %a:%d Reading EDID Info.\n", gEfiCallerBaseName, __FILE__, __LINE__));
    PCI_TYPE00 Pci;
    
    //
    // Read the PCI Configuration Header from the PCI Device to get BAR 0.
    //
    EFI_STATUS Status = Private->PciIo->Pci.Read(
        Private->PciIo, 
        EfiPciIoWidthUint32, 
        0, 
        sizeof(Pci) / sizeof(UINT32), 
        &Pci
    );

    if(EFI_ERROR(Status))
    {
        DbgPrint((DEBUG_INFO, "[%a]: %a:%d Error reading PCI Config: %r\n", gEfiCallerBaseName, __FILE__, __LINE__, Status));
        return Status;
    }

    // const volatile UINT8* SoftGpuBar0 = (volatile UINT8*) (UINTN) (Pci.Device.Bar[0] & ~0x7);

    UINT8 EdidData[EDID_BLOCK_SIZE * 2];

    // DbgPrint((DEBUG_INFO, "[%a]: %a:%d Performing PCIe Read of BAR 0.\n", gEfiCallerBaseName, __FILE__, __LINE__));
    Status = Private->PciIo->Mem.Read(
        Private->PciIo,
        EfiPciIoWidthUint32,
        0,
        0x3000,
        sizeof(EdidData) / sizeof(UINT32),
        EdidData
    );
    
    DbgPrint((DEBUG_INFO, "[%a]: %a:%d Finished PCIe Read of BAR 0.\n", gEfiCallerBaseName, __FILE__, __LINE__));

    if(EFI_ERROR(Status))
    {
        DbgPrint((DEBUG_INFO, "[%a]: %a:%d Error reading PCIe EDID Data: %r\n", gEfiCallerBaseName, __FILE__, __LINE__, Status));
        return Status;
    }

    // for(UINTN i = 0; i < EDID_BLOCK_SIZE * 2; ++i)
    // {
    //     EdidData[i] = SoftGpuBar0[i + 0x3000];
    // }

    //
    // Search for the EDID signature
    //
    UINT8* ValidEdid = &EdidData[0];
    UINT64 Signature = 0x00ffffffffffff00ull;
    UINTN Index;
    for(Index = 0; Index < EDID_BLOCK_SIZE * 2; ++Index, ++ValidEdid)
    {
        if(CompareMem(ValidEdid, &Signature, 8) == 0) 
        {
            break;
        }
    }

    if(Index == 256) 
    {
        //
        // No EDID signature found
        //
        return EFI_UNSUPPORTED;
    }

    *EdidDataBlock = AllocateCopyPool(
        EDID_BLOCK_SIZE,
        ValidEdid
    );

    if(*EdidDataBlock == NULL)
    {
        return EFI_OUT_OF_RESOURCES;
    }

    //
    // Currently only support EDID 1.x
    //
    *EdidSize = EDID_BLOCK_SIZE;

    return EFI_SUCCESS;
}

/**
 * Generate a search key for a specified timing data.
 *
 * @param EdidTiming Pointer to EDID timing
 *
 * @return The 32 bit unique key for search.
 *
 */
UINT32
CalculateEdidKey(
    EDID_TIMING* EdidTiming
)
{
    //
    // Be sure no conflicts for all standard timing defined by VESA.
    //
    return (EdidTiming->HorizontalResolution * 2) + EdidTiming->VerticalResolution;
}


/**
 * Search a specified Timing in all the valid EDID timings.
 *
 * @param ValidEdidTiming All valid EDID timing information.
 * @param EdidTiming The Timing to search for.
 *
 * @retval TRUE Found.
 * @retval FALSE Not found.
 *
 */
BOOLEAN
SearchEdidTiming(
    VALID_EDID_TIMING *ValidEdidTiming,
    EDID_TIMING       *EdidTiming
)
{
    UINT32 Key = CalculateEdidKey(EdidTiming);

    for(UINT32 i = 0; i < ValidEdidTiming->ValidNumber; ++i)
    {
        if(Key == ValidEdidTiming->Key[i])
        {
            return TRUE;
        }
    }

    return FALSE;
}

BOOLEAN
ParseEdidData(
    const UINT8* EdidBuffer,
    VALID_EDID_TIMING* ValidEdidTiming
)
{
    EDID_BLOCK* EdidDataBlock = (EDID_BLOCK*) EdidBuffer;

    UINT8 CheckSum = 0;
    for(UINT32 i = 0; i < EDID_BLOCK_SIZE; ++i)
    {
        CheckSum += (UINT8) EdidBuffer[i];
    }

    if(CheckSum != 0)
    {
        return FALSE;
    }

    SetMem(ValidEdidTiming, sizeof(ValidEdidTiming), 0);

    UINT32 ValidNumber = 0;

    if((EdidDataBlock->EstablishedTimings[0] != 0) || 
       (EdidDataBlock->EstablishedTimings[1] != 0) || 
       (EdidDataBlock->EstablishedTimings[2] != 0))
    {
        //
        // Established timing data
        //
        UINT32 TimingBits = EdidDataBlock->EstablishedTimings[0] | 
            (EdidDataBlock->EstablishedTimings[1] << 8) |
            ((EdidDataBlock->EstablishedTimings[2] & 0x80) << 9);

        for(UINT32 i = 0; i < VBE_EDID_ESTABLISHED_TIMING_MAX_NUMBER; ++i)
        {
            if(TimingBits & 0x01)
            {
                ValidEdidTiming->Key[ValidNumber] = CalculateEdidKey(&mVbeEstablishedEdidTiming[i]);
                ++ValidNumber;
            }
            TimingBits >>= 1;
        }
    }
    else
    {
        DbgPrint((DEBUG_INFO, "[%a]: %a:%d No established timings found\n", gEfiCallerBaseName, __FILE__, __LINE__));

        //
        // If no Established timing data, read the standard timing data
        //
        UINT8* BufferIndex = &EdidDataBlock->StandardTimingIdentification[0];
        for(UINT32 i = 0; i < 8; ++i)
        {
            if((BufferIndex[0] != 0x01) && (BufferIndex[1] != 0x01))
            {
                //
                // A valid Standard Timing
                //
                UINT16 HorizontalResolution = (UINT16) (BufferIndex[0] * 8 + 248);
                UINT8 AspectRatio = (UINT8) (BufferIndex[1] >> 6);
                UINT8 RefreshRate = (UINT8) ((BufferIndex[1] & 0x1F) + 60);
                
                UINT16 VerticalResolution;

                switch(AspectRatio)
                {
                    case 0:
                        VerticalResolution = (UINT16) (HorizontalResolution / 16 * 10);
                        break;
                    case 1:
                        VerticalResolution = (UINT16) (HorizontalResolution / 4 * 3);
                        break;
                    case 2:
                        VerticalResolution = (UINT16) (HorizontalResolution / 5 * 4);
                        break;
                    case 3:
                        VerticalResolution = (UINT16) (HorizontalResolution / 16 * 9);
                        break;
                    default:
                        VerticalResolution = (UINT16) (HorizontalResolution / 4 * 3);
                        break;
                }
                
                EDID_TIMING TempTiming;
                TempTiming.HorizontalResolution = HorizontalResolution;
                TempTiming.VerticalResolution = VerticalResolution;
                TempTiming.RefreshRate = RefreshRate;
                ValidEdidTiming->Key[ValidNumber] = CalculateEdidKey(&TempTiming);
                ++ValidNumber;
            }

            BufferIndex += 2;
        }
    }

    ValidEdidTiming->ValidNumber = ValidNumber;
    return TRUE;
}

EFI_STATUS
SoftGpuVideoModeSetup(
    SOFT_GPU_PRIVATE_DATA* Private
)
{
    Private->EdidDiscovered.Edid = NULL;
    Private->EdidDiscovered.SizeOfEdid = 0;
    Private->EdidActive.Edid = NULL;
    Private->EdidActive.SizeOfEdid = 0;

    BOOLEAN EdidFound = FALSE;
    BOOLEAN EdidOverrideFound = FALSE;
    BOOLEAN TimingMatch = FALSE;
    
    UINT32 EdidAttributes = 0xFF;
    UINTN EdidOverrideDataSize = 0;
    UINT8* EdidOverrideDataBlock = NULL;
    UINTN EdidDiscoveredDataSize = 0;
    UINT8* EdidDiscoveredDataBlock = NULL;
    UINTN EdidActiveDataSize = 0;
    UINT8 *EdidActiveDataBlock = NULL;
    UINT32 ValidModeCount = 0;

    EFI_EDID_OVERRIDE_PROTOCOL* EdidOverride;
    VALID_EDID_TIMING ValidEdidTiming;
    SOFT_GPU_VGA_MODE_DATA* ModeData = NULL;
    const SOFT_GPU_VGA_VIDEO_MODES* VideoMode = NULL;
    EDID_TIMING TempTiming;

    //
    // Find EDID Override protocol first, this protocol is installed by platform if needed.
    //
    EFI_STATUS Status = gBS->LocateProtocol(
        &gEfiEdidOverrideProtocolGuid,
        NULL,
        (VOID**) &EdidOverride
    );

    // DbgPrint((DEBUG_INFO, "[%a]: %a:%d EDID Override Protocol Status: %r\n", gEfiCallerBaseName, __FILE__, __LINE__, Status));

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
        //
        // If EDID Override data doesn't exist or EFI_EDID_OVERRIDE_DONT_OVERRIDE returned,
        // read EDID information through BAR 0
        //
        Status = ReadEdidData(Private, &EdidDiscoveredDataBlock, &EdidDiscoveredDataSize);

        DbgPrint((DEBUG_INFO, "[%a]: %a:%d Read EDID Status: %r\n", gEfiCallerBaseName, __FILE__, __LINE__, Status));

        if(!EFI_ERROR(Status))
        {
            Private->EdidDiscovered.SizeOfEdid = (UINT32) EdidDiscoveredDataSize;
            Private->EdidDiscovered.Edid = (UINT8*) AllocateCopyPool(
                EdidDiscoveredDataSize,
                EdidDiscoveredDataBlock
            );
            if(Private->EdidDiscovered.Edid == NULL)
            {
                Status = EFI_OUT_OF_RESOURCES;
                goto Done;
            }

            EdidActiveDataSize = Private->EdidDiscovered.SizeOfEdid;
            EdidActiveDataBlock = Private->EdidDiscovered.Edid;

            EdidFound = TRUE;
        }
        else
        {
            DbgPrint((DEBUG_INFO, "[%a]: %a:%d Error Reading EDID Info: %r\n", gEfiCallerBaseName, __FILE__, __LINE__, Status));
        }
    }

    if(!EdidFound && EdidOverrideFound)
    {
        EdidActiveDataSize = EdidOverrideDataSize;
        EdidActiveDataBlock = EdidOverrideDataBlock;
        EdidFound = TRUE;
    }

    if(EdidFound)
    {
        //
        // Parse EDID data structure to retrieve modes supported by monitor
        //
        if(ParseEdidData((UINT8*) EdidActiveDataBlock, &ValidEdidTiming) == TRUE) 
        {
            //
            // Copy EDID Override Data to EDID Active Data
            //
            Private->EdidActive.SizeOfEdid = (UINT32) EdidActiveDataSize;
            Private->EdidActive.Edid = (UINT8*) AllocateCopyPool(
                EdidActiveDataSize,
                EdidActiveDataBlock
            );

            if(Private->EdidActive.Edid == NULL) 
            {
                Status = EFI_OUT_OF_RESOURCES;
                goto Done;
            }
        }
        else
        {
            DbgPrint((DEBUG_INFO, "[%a]: %a:%d Failed to parse EDID info\n", gEfiCallerBaseName, __FILE__, __LINE__));
        }
    }
    else
    {
        Private->EdidActive.SizeOfEdid = 0;
        Private->EdidActive.Edid = NULL;
    }

    if(EdidFound)
    {
        ValidModeCount = 0;
        Private->ModeData = AllocatePool(sizeof(SOFT_GPU_VGA_MODE_DATA) * 69);
        ModeData = &Private->ModeData[0];
        VideoMode = &gSoftGpuGopVideoModes[0];

        for(UINTN i = 0; i < gSoftGpuGopVideoModeCount; ++i, ++VideoMode)
        {
            TimingMatch = TRUE;

            //
            // Check whether match with VBoxVga video mode
            //
            TempTiming.HorizontalResolution = (UINT16) VideoMode->Width;
            TempTiming.VerticalResolution = (UINT16) VideoMode->Height;
            TempTiming.RefreshRate = (UINT16) VideoMode->RefreshRate;

            if(SearchEdidTiming(&ValidEdidTiming, &TempTiming) != TRUE)
            {
                TimingMatch = FALSE;
            }

            //
            // Not export Mode 0x0 as GOP mode, this is not defined in spec.
            //
            if(VideoMode->Width == 0 || VideoMode->Height == 0)
            {
                TimingMatch = FALSE;
            }
            
            //
            // Check whether the mode would be exceeding the VRAM size.
            //
            if(VideoMode->Width * VideoMode->Height * (VideoMode->ColorDepth / 8) > Private->VRAMSize)
            {
                TimingMatch = FALSE;
            }

            if(TimingMatch)
            {
                ModeData->ModeNumber = (UINT32) i;
                ModeData->HorizontalResolution = VideoMode->Width;
                ModeData->VerticalResolution = VideoMode->Height;
                ModeData->ColorDepth = VideoMode->ColorDepth;
                ModeData->RefreshRate = VideoMode->RefreshRate;

                ++ModeData;
                ++ValidModeCount;
            }
        }
    }
    else
    {
        Status = EFI_NOT_FOUND;
        goto Done;
    }

    // Sort list of video modes (keeping duplicates) by increasing X, then Y,
    // then the mode number. This way the custom modes are not overriding the
    // default modes if they are for the same resolution.
    ModeData = &Private->ModeData[0];

    for(UINTN i = 0; i < ValidModeCount - 1; ++i, ++ModeData)
    {
        SOFT_GPU_VGA_MODE_DATA* ModeData2 = ModeData + 1;
        for(UINTN j = i + 1; j < ValidModeCount; ++j, ++ModeData2)
        {
            if(
                ModeData->HorizontalResolution > ModeData2->HorizontalResolution ||
                (ModeData->HorizontalResolution == ModeData2->HorizontalResolution &&
                 ModeData->VerticalResolution > ModeData2->VerticalResolution) ||
                (ModeData->HorizontalResolution == ModeData2->HorizontalResolution &&
                 ModeData->VerticalResolution == ModeData2->VerticalResolution &&
                 ModeData->ModeNumber > ModeData2->ModeNumber)
            )
            {
                SOFT_GPU_VGA_MODE_DATA Temp;
                (void) CopyMem(&Temp, ModeData, sizeof(Temp));
                (void) CopyMem(ModeData, ModeData2, sizeof(ModeData));
                (void) CopyMem(ModeData2, &Temp, sizeof(ModeData2));
                DbgPrint((DEBUG_INFO, "[%a]: %a:%d Swapped Mode Entries %d and %d\n", gEfiCallerBaseName, __FILE__, __LINE__, i, j));
            }
        }
    }

    // dump mode list for debugging purposes
    ModeData = &Private->ModeData[0];
    for(UINTN i = 0; i < ValidModeCount; ++i, ++ModeData) 
    {
        DbgPrint((DEBUG_INFO, "[%a]: %a:%d Mode %d: %dx%d Mode Number: %d\n", gEfiCallerBaseName, __FILE__, __LINE__, i, ModeData->HorizontalResolution, ModeData->VerticalResolution, ModeData->ModeNumber));
    }

    Private->MaxMode = ValidModeCount;

    if(EdidOverrideDataBlock)
    {
        FreePool(EdidOverrideDataBlock);
    }

    return EFI_SUCCESS;

Done:
    if(EdidOverrideDataBlock != NULL) 
    {
        FreePool (EdidOverrideDataBlock);
    }

    if(Private->EdidDiscovered.Edid != NULL)
    {
        FreePool (Private->EdidDiscovered.Edid);
    }

    return EFI_DEVICE_ERROR;
}

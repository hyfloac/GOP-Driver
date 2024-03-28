/**
 * Sourced from VirtualBox's VBoxPkg UEFI driver.
 */
#pragma once

#include <Base.h>
#include <Library/BaseLib.h>
#include <Library/PrintLib.h>
#include <Library/DebugLib.h>

#include <Protocol/DevicePath.h>
#include <Protocol/DevicePathToText.h>
#include <Uefi/UefiSpec.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Uefi/UefiBaseType.h>

/** The base of the I/O ports used for interaction between the EFI firmware and DevEFI. */
#define EFI_PORT_BASE           0xEF10  /**< @todo r=klaus stupid choice which causes trouble with PCI resource allocation in complex bridge setups, change to 0x0400 with appropriate saved state and reset handling */
/** The number of ports. */
#define EFI_PORT_COUNT          0x0008

/** Debug logging.
 * The chars written goes to the log.
 * Reading has no effect.
 * @remarks The port number is the same as on of those used by the PC BIOS. */
#define EFI_DEBUG_PORT          (EFI_PORT_BASE+0x3)


UINTN VBoxPrintChar(int ch);
UINTN VBoxPrintGuid(CONST EFI_GUID *pGuid);
UINTN VBoxPrintHex(UINT64 uValue, UINTN cbType);
UINTN VBoxPrintHexDump(const void *pv, UINTN cb);
UINTN VBoxPrintString(const char *pszString);

void EFIAPI VBoxDebugPrint(IN UINTN ErrorLevel, IN CONST CHAR8 *Format, ...);

/**
 * Sourced from VirtualBox's VBoxPkg UEFI driver.
 */
#include "VirtualBoxDebugLog.h"

typedef UINT16                RTIOPORT;

#define RT_INLINE_ASM_USES_INTRIN 1

/**
 * Writes a 8-bit unsigned integer to an I/O port, ordered.
 *
 * @param   Port    I/O port to write to.
 * @param   u8      8-bit integer to write.
 */
#if RT_INLINE_ASM_EXTERNAL && !RT_INLINE_ASM_USES_INTRIN
RT_ASM_DECL_PRAGMA_WATCOM(void) ASMOutU8(RTIOPORT Port, UINT8 u8);
#else
inline void ASMOutU8(RTIOPORT Port, UINT8 u8)
{
# if RT_INLINE_ASM_GNU_STYLE
    __asm__ __volatile__("outb %b1, %w0\n\t"
                         :: "Nd" (Port),
                            "a" (u8));

# elif RT_INLINE_ASM_USES_INTRIN
    __outbyte(Port, u8);

# else
    __asm
    {
        mov     dx, [Port]
        mov     al, [u8]
        out     dx, al
    }
# endif
}
#endif


/**
 * Writes a 16-bit unsigned integer to an I/O port, ordered.
 *
 * @param   Port    I/O port to write to.
 * @param   u16     16-bit integer to write.
 */
#if RT_INLINE_ASM_EXTERNAL && !RT_INLINE_ASM_USES_INTRIN
RT_ASM_DECL_PRAGMA_WATCOM(void) ASMOutU16(RTIOPORT Port, UINT16 u16);
#else
inline void ASMOutU16(RTIOPORT Port, UINT16 u16)
{
# if RT_INLINE_ASM_GNU_STYLE
    __asm__ __volatile__("outw %w1, %w0\n\t"
                         :: "Nd" (Port),
                            "a" (u16));

# elif RT_INLINE_ASM_USES_INTRIN
    __outword(Port, u16);

# else
    __asm
    {
        mov     dx, [Port]
        mov     ax, [u16]
        out     dx, ax
    }
# endif
}
#endif


/**
 * Writes a 32-bit unsigned integer to an I/O port, ordered.
 *
 * @param   Port    I/O port to write to.
 * @param   u32     32-bit integer to write.
 */
#if RT_INLINE_ASM_EXTERNAL && !RT_INLINE_ASM_USES_INTRIN
RT_ASM_DECL_PRAGMA_WATCOM(void) ASMOutU32(RTIOPORT Port, UINT32 u32);
#else
inline void ASMOutU32(RTIOPORT Port, UINT32 u32)
{
# if RT_INLINE_ASM_GNU_STYLE
    __asm__ __volatile__("outl %1, %w0\n\t"
                         :: "Nd" (Port),
                            "a" (u32));

# elif RT_INLINE_ASM_USES_INTRIN
    __outdword(Port, u32);

# else
    __asm
    {
        mov     dx, [Port]
        mov     eax, [u32]
        out     dx, eax
    }
# endif
}
#endif

/**
 * Writes a string of 8-bit unsigned integer items to an I/O port, ordered.
 *
 * @param   Port    I/O port to write to.
 * @param   pau8    Pointer to the string buffer.
 * @param   c       The number of items to write.
 */
#if RT_INLINE_ASM_EXTERNAL && !RT_INLINE_ASM_USES_INTRIN
RT_ASM_DECL_PRAGMA_WATCOM(void) ASMOutStrU8(RTIOPORT Port, UINT8 const RT_FAR *pau8, UINTN c);
#else
inline void ASMOutStrU8(RTIOPORT Port, UINT8 const *pau8, UINTN c)
{
# if RT_INLINE_ASM_GNU_STYLE
    __asm__ __volatile__("rep; outsb\n\t"
                         : "+S" (pau8),
                           "+c" (c)
                         : "d" (Port));

# elif RT_INLINE_ASM_USES_INTRIN
    __outbytestring(Port, (unsigned char *)pau8, (unsigned long)c);

# else
    __asm
    {
        mov     dx, [Port]
        mov     ecx, [c]
        mov     eax, [pau8]
        xchg    esi, eax
        rep outsb
        xchg    esi, eax
    }
# endif
}
#endif

/**
 * Prints a char.
 * @returns 1
 * @param   ch              The char to print.
 */
UINTN VBoxPrintChar(int ch)
{
    ASMOutU8(EFI_DEBUG_PORT, (UINT8) ch);
    return 1;
}

/**
 * Prints a EFI GUID.
 *
 * @returns Number of bytes printed.
 *
 * @param   pGuid           The GUID to print
 */
UINTN VBoxPrintGuid(CONST EFI_GUID *pGuid)
{
    VBoxPrintHex(pGuid->Data1, sizeof(pGuid->Data1));
    VBoxPrintChar('-');
    VBoxPrintHex(pGuid->Data2, sizeof(pGuid->Data2));
    VBoxPrintChar('-');
    VBoxPrintHex(pGuid->Data3, sizeof(pGuid->Data3));
    VBoxPrintChar('-');
    VBoxPrintHex(pGuid->Data4[0], sizeof(pGuid->Data4[0]));
    VBoxPrintHex(pGuid->Data4[1], sizeof(pGuid->Data4[1]));
    VBoxPrintChar('-');
    VBoxPrintHex(pGuid->Data4[2], sizeof(pGuid->Data4[2]));
    VBoxPrintHex(pGuid->Data4[3], sizeof(pGuid->Data4[3]));
    VBoxPrintHex(pGuid->Data4[4], sizeof(pGuid->Data4[4]));
    VBoxPrintHex(pGuid->Data4[5], sizeof(pGuid->Data4[5]));
    VBoxPrintHex(pGuid->Data4[6], sizeof(pGuid->Data4[6]));
    VBoxPrintHex(pGuid->Data4[7], sizeof(pGuid->Data4[7]));
    return 37;
}

/**
 * Prints a char.
 * @param   ch              The char to print.
 */
inline void vboxPrintHexChar(int ch)
{
    ASMOutU8(EFI_DEBUG_PORT, (UINT8) ch);
}

/**
 * Print a hex number, up to 64-bit long.
 *
 * @returns Number of chars printed.
 *
 * @param   uValue          The value.
 * @param   cbType          The size of the value type.
 */
UINTN VBoxPrintHex(UINT64 uValue, UINTN cbType)
{
    static const char s_szHex[17] = "0123456789abcdef";
    switch (cbType)
    {
/*
 * We have to cast the result to UINTN before indexing into the array
 * or cl.exe insists on generating a call to __allmul for unoptimized 32bit builds,
 * see: https://patchew.org/EDK2/1486606121-226912-1-git-send-email-dandan.bi@intel.com/
 */
#define VAL_NIBBLE_EXTRACT(a_uValue, a_iNibbleStart) (s_szHex[(UINTN)(RShiftU64((a_uValue), (a_iNibbleStart)) & 0xf)])
        case 8:
            vboxPrintHexChar(VAL_NIBBLE_EXTRACT(uValue, 60));
            vboxPrintHexChar(VAL_NIBBLE_EXTRACT(uValue, 56));
            vboxPrintHexChar(VAL_NIBBLE_EXTRACT(uValue, 52));
            vboxPrintHexChar(VAL_NIBBLE_EXTRACT(uValue, 48));
            vboxPrintHexChar(VAL_NIBBLE_EXTRACT(uValue, 44));
            vboxPrintHexChar(VAL_NIBBLE_EXTRACT(uValue, 40));
            vboxPrintHexChar(VAL_NIBBLE_EXTRACT(uValue, 36));
            vboxPrintHexChar(VAL_NIBBLE_EXTRACT(uValue, 32));
        case 4:
            vboxPrintHexChar(VAL_NIBBLE_EXTRACT(uValue, 28));
            vboxPrintHexChar(VAL_NIBBLE_EXTRACT(uValue, 24));
            vboxPrintHexChar(VAL_NIBBLE_EXTRACT(uValue, 20));
            vboxPrintHexChar(VAL_NIBBLE_EXTRACT(uValue, 16));
        case 2:
            vboxPrintHexChar(VAL_NIBBLE_EXTRACT(uValue, 12));
            vboxPrintHexChar(VAL_NIBBLE_EXTRACT(uValue,  8));
        case 1:
            vboxPrintHexChar(VAL_NIBBLE_EXTRACT(uValue,  4));
            vboxPrintHexChar(VAL_NIBBLE_EXTRACT(uValue,  0));
            break;
#undef VAL_NIBBLE_EXTRACT
    }

#if 0 /* There is no MultU32x32 for 32bit and cl insists on emitting __allmul otherwise so we just hardcode everything here... */
    return cbType * 2;
#else
    static UINTN s_acbPrinted[9] = { 0, 2, 4, 0, 8, 0, 0, 0, 16};
    if (cbType < sizeof(s_acbPrinted) / sizeof(s_acbPrinted[0]))
        return s_acbPrinted[cbType];
    return 0;
#endif
}


/**
 * Prints a char.
 * @returns 1.
 * @param   ch              The char to print.
 */
inline int vboxPrintHexDumpChar(int ch)
{
    ASMOutU8(EFI_DEBUG_PORT, (UINT8) ch);
    return 1;
}


/**
 * Prints a hex dump the specified memory block.
 *
 * @returns Number of bytes printed.
 *
 * @param   pv      The memory to dump.
 * @param   cb      Number of bytes to dump.
 */
UINTN VBoxPrintHexDump(const void *pv, UINTN cb)
{
    UINTN          cchPrinted = 0;
    UINT8 const  *pb         = (UINT8 const *)pv;
    while (cb > 0)
    {
        unsigned i;

        /* the offset */
        cchPrinted += VBoxPrintHex((UINTN)pb, sizeof(pb));
        cchPrinted += VBoxPrintString("  ");

        /* the hex bytes value. */
        for (i = 0; i < 16; i++)
        {
            cchPrinted += vboxPrintHexDumpChar(i == 7 ? '-' : ' ');
            if (i < cb)
                cchPrinted += VBoxPrintHex(pb[i], 1);
            else
                cchPrinted += VBoxPrintString("  ");
        }

        /* the printable chars */
        cchPrinted += VBoxPrintString("  ");
        for (i = 0; i < 16 && i < cb; i++)
            cchPrinted += vboxPrintHexDumpChar((pb[i] - 0x20 < 95U)
                                               ? pb[i]
                                               : '.');

        /* finally, the new line. */
        cchPrinted += vboxPrintHexDumpChar('\n');

        /*
         * Advance.
         */
        if (cb <= 16)
            break;
        cb -= 16;
        pb += 16;
    }

    return cchPrinted;
}


/**
 * Prints a string to the EFI debug port.
 *
 * @returns The string length.
 * @param   pszString       The string to print.
 */
UINTN VBoxPrintString(const char *pszString)
{
    const char *pszEnd = pszString;
    while (*pszEnd)
        pszEnd++;
    ASMOutStrU8(EFI_DEBUG_PORT, (UINT8 const *)pszString, pszEnd - pszString);
    return pszEnd - pszString;
}

VOID EFIAPI
VBoxDebugPrint(IN UINTN ErrorLevel, IN CONST CHAR8 *Format, ...)
{
    CHAR8       szBuf[256];
    VA_LIST     va;
    UINTN       cch;
    BOOLEAN     InterruptState;

    /* No pool noise, please. */
    if (ErrorLevel == DEBUG_POOL)
        return;

    VA_START(va, Format);
    cch = AsciiVSPrint(szBuf, sizeof(szBuf), Format, va);
    VA_END(va);

    /* make sure it's terminated and doesn't end with a newline */
    if (cch >= sizeof(szBuf))
        cch = sizeof(szBuf) - 1;
    while (cch > 0 && (szBuf[cch - 1] == '\n' || szBuf[cch - 1] == '\r'))
        cch--;
    szBuf[cch] = '\0';

    InterruptState = SaveAndDisableInterrupts();

    /* Output the log string. */
    VBoxPrintString("dbg/");
    VBoxPrintHex(ErrorLevel, sizeof(ErrorLevel));
    VBoxPrintChar(' ');
    VBoxPrintString(szBuf);
    VBoxPrintChar('\n');

    SetInterruptState(InterruptState);
}

/* dlpatch.h - the numbers for the Break Damage Limit patch (pure C, unit-testable).
 *
 * The game builds the per-hit damage limit like this (FFX.exe, 32-bit):
 *
 *   mov  eax, 0x800
 *   and  ax, [ebx+0x6BE]          ; does the attacker have the Break Damage Limit bit?
 *   movzx ebx, ax
 *   movzx eax, word ptr [esi+0x20]
 *   neg  ebx
 *   sbb  ebx, ebx                 ; ebx = 0xFFFFFFFF if the bit was set, else 0
 *   and  ebx, 0x15F90             ; imm32 #1: 90,000 if set, else 0      <-- patched
 *   add  ebx, 0x270F              ; + 9,999   => limit 99,999 or 9,999
 *   test al, al
 *   jns  skip
 *   mov  ebx, 0x1869F             ; imm32 #2: moves that break the limit by themselves <-- patched
 *   ...
 *
 * Patch: imm32 #1 = limit - 9999, imm32 #2 = limit.  With limit = 999,999 the limit
 * becomes 9,999 + 990,000 = 999,999 for anyone with the bit (and 9,999 for everyone else).
 */
#ifndef DLPATCH_H
#define DLPATCH_H

#define DL_OFF_EXTRA   0x19      /* match + 0x19 : imm32 of  and ebx,imm32  (90,000)  */
#define DL_OFF_FORCED  0x28      /* match + 0x28 : imm32 of  mov ebx,imm32  (99,999)  */

#define DL_VANILLA_BASE  9999u
#define DL_DEFAULT_LIMIT 999999u
#define DL_MIN_LIMIT     10000u
#define DL_MAX_LIMIT     999999u

static int dl_limit_ok(unsigned long v)
{
    return v >= DL_MIN_LIMIT && v <= DL_MAX_LIMIT;
}

/* the two immediates for a chosen limit */
static void dl_values(unsigned limit, unsigned *extra, unsigned *forced)
{
    *extra  = limit - DL_VANILLA_BASE;
    *forced = limit;
}

/* little-endian dword, as stored in the code */
static void dl_put32(unsigned char *out, unsigned v)
{
    out[0] = (unsigned char)(v & 0xFF);
    out[1] = (unsigned char)((v >> 8) & 0xFF);
    out[2] = (unsigned char)((v >> 16) & 0xFF);
    out[3] = (unsigned char)((v >> 24) & 0xFF);
}
#endif

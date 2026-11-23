/*
 * ffx_true_bdl.c  --  True Break Damage Limit (Break Damage Limit: 99,999 -> 999,999)
 *
 * Drop-in module for ffgriever's FF10 Module Loader (works with UnX and the
 * External File Loader). Put ffx_true_bdl.dll in <game>\modules\.
 * Remove the DLL to remove the patch.
 *
 * What it changes: the per-hit damage limit used when the attacker has Break Damage
 * Limit. The normal 9,999 limit for everyone else is NOT changed.
 * The hook is found by AOB scan at startup, so it follows small code shifts.
 * Settings: modules\ffx_true_bdl.ini (created on first run).
 * Log: ffx_true_bdl.log next to FFX.exe.
 *
 * Idea: Beelzepog.  Research and testing: Shro.
 * Guidance and code: Claude (Anthropic).  Module loader: ffgriever.
 *
 * MUST be built as a 32-bit (x86) DLL - FFX.exe is a 32-bit process.
 */
#define _CRT_SECURE_NO_WARNINGS
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <stdio.h>
#include <stdarg.h>
#include <string.h>

#if !defined(_M_IX86) && !defined(__i386__)
#error "ffx_true_bdl must be built as a 32-bit (x86) DLL"
#endif

#include "scanner.h"
#include "dlpatch.h"

/* The 44 bytes that build the damage limit (see dlpatch.h). Every byte is fixed:
 * constants and structure offsets only, no absolute addresses. */
static const char DL_HOOK_AOB[] =
    "B8 00 08 00 00 66 23 83 BE 06 00 00 0F B7 D8 0F B7 46 20 F7 DB 1B DB "
    "81 E3 90 5F 01 00 81 C3 0F 27 00 00 84 C0 79 07 BB 9F 86 01 00";
#define DL_EXPECTED_OFFSET 0x38ECCAUL    /* only used to flag a shifted build in the log */

static HMODULE g_self;
static FILE   *g_log;
static DWORD   g_t0;
static int     g_done;
static char    g_ini[MAX_PATH];

static void say(const char *fmt, ...)
{
    va_list ap;
    if (!g_log) return;
    fprintf(g_log, "[%6lu ms] ", (unsigned long)(GetTickCount() - g_t0));
    va_start(ap, fmt);
    vfprintf(g_log, fmt, ap);
    va_end(ap);
    fputc('\n', g_log);
    fflush(g_log);
}

/* "<dir of the exe or of this dll>\<name>" */
static int make_path(char *out, size_t cap, int use_exe, const char *name)
{
    char *slash;
    DWORD n = GetModuleFileNameA(use_exe ? NULL : g_self, out, (DWORD)cap);
    if (n == 0 || n >= cap) return 0;
    slash = strrchr(out, '\\');
    if (!slash || (size_t)(slash + 1 - out) + strlen(name) + 1 >= cap) return 0;
    strcpy(slash + 1, name);
    return 1;
}

static void ensure_ini(void)
{
    FILE *f;
    if (!make_path(g_ini, sizeof g_ini, 0, "ffx_true_bdl.ini")) { g_ini[0] = 0; return; }
    if (GetFileAttributesA(g_ini) != INVALID_FILE_ATTRIBUTES) return;
    f = fopen(g_ini, "w");
    if (!f) return;
    fputs("; True Break Damage Limit - the damage limit for attackers WITH Break Damage Limit.\n"
          "; Everyone else keeps the normal 9999 limit. Allowed range 10000..999999.\n"
          "; Restart the game after changing it.\n"
          "; Idea: Beelzepog. Research and testing: Shro. Guidance and code: Claude (Anthropic).\n"
          "[Damage]\n"
          "BreakDamageLimit=999999\n", f);
    fclose(f);
}

static unsigned read_limit(void)
{
    unsigned long v = DL_DEFAULT_LIMIT;
    if (g_ini[0]) v = GetPrivateProfileIntA("Damage", "BreakDamageLimit", (int)DL_DEFAULT_LIMIT, g_ini);
    if (!dl_limit_ok(v)) {
        say("BreakDamageLimit=%lu is outside %u..%u - using %u", v, DL_MIN_LIMIT, DL_MAX_LIMIT, DL_DEFAULT_LIMIT);
        v = DL_DEFAULT_LIMIT;
    }
    return (unsigned)v;
}

static int patch_code(void *dst, const void *src, size_t n)
{
    DWORD old;
    if (!VirtualProtect(dst, n, PAGE_EXECUTE_READWRITE, &old)) return 0;
    memcpy(dst, src, n);
    VirtualProtect(dst, n, old, &old);
    FlushInstructionCache(GetCurrentProcess(), dst, n);
    return 1;
}

/* Two in-place dword changes. All or nothing: if the second write fails the first is undone. */
static int apply_damage_limit(BYTE *match, unsigned limit)
{
    static const BYTE exp1[4] = { 0x90, 0x5F, 0x01, 0x00 };       /* 90,000 */
    static const BYTE exp2[4] = { 0x9F, 0x86, 0x01, 0x00 };       /* 99,999 */
    BYTE *p1 = match + DL_OFF_EXTRA, *p2 = match + DL_OFF_FORCED;
    BYTE old1[4], n1[4], n2[4];
    unsigned extra, forced;

    if (memcmp(p1, exp1, 4) != 0 || memcmp(p2, exp2, 4) != 0) return 0;   /* sanity */
    memcpy(old1, p1, 4);
    dl_values(limit, &extra, &forced);
    dl_put32(n1, extra);
    dl_put32(n2, forced);

    if (!patch_code(p1, n1, 4)) return 0;
    if (!patch_code(p2, n2, 4)) { patch_code(p1, old1, 4); return 0; }
    return 1;
}

static int range_readable(const BYTE *p, size_t size)
{
    const BYTE *end = p + size;
    while (p < end) {
        MEMORY_BASIC_INFORMATION mbi;
        if (!VirtualQuery(p, &mbi, sizeof(mbi))) return 0;
        if (mbi.State != MEM_COMMIT) return 0;
        if (mbi.Protect & (PAGE_NOACCESS | PAGE_GUARD)) return 0;
        p = (const BYTE *)mbi.BaseAddress + mbi.RegionSize;
    }
    return 1;
}

/* Scan every executable section. Returns hit count, or -1 if code isn't readable yet. */
static int scan_exe(HMODULE mod, const BYTE *val, const BYTE *msk, int n, DWORD *hits, int maxhits)
{
    const BYTE *base = (const BYTE *)mod;
    const IMAGE_DOS_HEADER *dos = (const IMAGE_DOS_HEADER *)base;
    const IMAGE_NT_HEADERS *nt  = (const IMAGE_NT_HEADERS *)(base + dos->e_lfanew);
    const IMAGE_SECTION_HEADER *sec = IMAGE_FIRST_SECTION(nt);
    int total = 0, i;

    for (i = 0; i < nt->FileHeader.NumberOfSections; i++) {
        const BYTE *start;
        DWORD size;
        size_t local[8];
        int found, k;

        if (!(sec[i].Characteristics & IMAGE_SCN_MEM_EXECUTE)) continue;
        start = base + sec[i].VirtualAddress;
        size  = sec[i].Misc.VirtualSize;
        if (!range_readable(start, size)) return -1;

        found = pat_scan(start, size, val, msk, n, local, 8);
        for (k = 0; k < found && k < 8; k++)
            if (total + k < maxhits)
                hits[total + k] = (DWORD)(start - base) + (DWORD)local[k];
        total += found;
    }
    return total;
}

/* One attempt. Sets g_done when the outcome is final. */
static void attempt(int timed_out)
{
    BYTE val[PAT_MAX], msk[PAT_MAX];
    DWORD hits[8];
    HMODULE exe = GetModuleHandleA(NULL);
    int n = pat_parse(DL_HOOK_AOB, val, msk, PAT_MAX), c, k;

    if (n < 0) { say("BAD PATTERN (bug in the module)"); g_done = 1; return; }
    c = scan_exe(exe, val, msk, n, hits, 8);

    if (c == 1) {
        unsigned limit = read_limit();
        say("hook found at FFX.exe+%06lX%s", (unsigned long)hits[0],
            hits[0] == DL_EXPECTED_OFFSET ? "" : "  (different from the build this was made on - a game update?)");
        if (apply_damage_limit((BYTE *)exe + hits[0], limit))
            say("PATCHED - Break Damage Limit cap is now %u (normal limit stays 9999)", limit);
        else
            say("PATCH FAILED - no change made");
        g_done = 1;
    } else if (c > 1) {
        say("AMBIGUOUS: %d matches - not patching", c);
        for (k = 0; k < c && k < 8; k++) say("   FFX.exe+%06lX", (unsigned long)hits[k]);
        g_done = 1;
    } else if (timed_out) {
        say(c < 0 ? "NOT READY: code never became readable - no change made"
                  : "NOT FOUND: hook pattern not in this exe (already patched by another module or "
                    "Cheat Engine, or the game changed) - no change made");
        g_done = 1;
    }
}

static void finish(void)
{
    say("finished");
    if (g_log) { fclose(g_log); g_log = NULL; }
}

/* Only runs if the first (synchronous) attempt could not resolve it. */
static DWORD WINAPI retry_worker(LPVOID arg)
{
    (void)arg;
    while (!g_done) {
        Sleep(500);
        attempt((GetTickCount() - g_t0) > 90000UL);
    }
    finish();
    return 0;
}

BOOL WINAPI DllMain(HINSTANCE inst, DWORD reason, LPVOID reserved)
{
    (void)reserved;
    if (reason == DLL_PROCESS_ATTACH) {
        char path[MAX_PATH];
        g_self = (HMODULE)inst;
        DisableThreadLibraryCalls(inst);

        g_t0 = GetTickCount();
        if (make_path(path, sizeof path, 1, "ffx_true_bdl.log")) g_log = fopen(path, "w");
        ensure_ini();
        say("ffx_true_bdl loaded (True Break Damage Limit)");

        /* First attempt runs here, while the game's main thread is still inside the loader
         * and not executing game code, so patching is race-free. */
        attempt(0);

        if (!g_done) {
            HANDLE t = CreateThread(NULL, 0, retry_worker, NULL, 0, NULL);
            if (t) CloseHandle(t); else finish();
        } else {
            finish();
        }
    }
    return TRUE;
}

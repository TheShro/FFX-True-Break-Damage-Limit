/* scanner.h - tiny wildcard byte-pattern scanner (pure C, no OS calls). */
#ifndef SCANNER_H
#define SCANNER_H
#include <stddef.h>

#define PAT_MAX 128

static int pat_hex(char c)
{
    if (c >= '0' && c <= '9') return c - '0';
    if (c >= 'a' && c <= 'f') return c - 'a' + 10;
    if (c >= 'A' && c <= 'F') return c - 'A' + 10;
    return -1;
}

/* Parse "0F B6 ?? 19" into value/mask arrays (mask 1 = must match).
 * Returns the pattern length, or -1 on a malformed pattern. */
static int pat_parse(const char *s, unsigned char *val, unsigned char *msk, int max)
{
    int n = 0;
    while (*s) {
        int hi, lo;
        if (*s == ' ' || *s == '\t') { s++; continue; }
        if (n >= max) return -1;
        if (*s == '?') {
            val[n] = 0; msk[n] = 0; n++;
            s++;
            if (*s == '?') s++;
            continue;
        }
        hi = pat_hex(s[0]);
        lo = (hi < 0) ? -1 : pat_hex(s[1]);
        if (hi < 0 || lo < 0) return -1;
        val[n] = (unsigned char)(hi * 16 + lo);
        msk[n] = 1;
        n++;
        s += 2;
    }
    return n;
}

/* Count matches in buf[0..size). Stores the first `maxhits` offsets. */
static int pat_scan(const unsigned char *buf, size_t size,
                    const unsigned char *val, const unsigned char *msk, int n,
                    size_t *hits, int maxhits)
{
    size_t off;
    int count = 0;
    if (n <= 0 || size < (size_t)n) return 0;
    for (off = 0; off + (size_t)n <= size; off++) {
        int j = 0;
        while (j < n && (!msk[j] || buf[off + j] == val[j])) j++;
        if (j == n) {
            if (count < maxhits) hits[count] = off;
            count++;
        }
    }
    return count;
}
#endif

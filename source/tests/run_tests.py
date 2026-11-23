#!/usr/bin/env python3
"""Runs the game's real limit-building instructions (the 44 bytes the module scans for), patched with the
values dlpatch.h produces, natively (32-bit, no libc) and checks the limit for every case.
Needs gcc, as and ld.  Run from the project folder:  python3 tests/run_tests.py"""
import re, os, subprocess, sys, tempfile
ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
src  = open(os.path.join(ROOT,'ffx_true_bdl.c')).read()
pat  = ''.join(re.findall(r'"([0-9A-F ]+)"', src.split('DL_HOOK_AOB[] =')[1].split(';')[0])).split()
code = bytes(int(x,16) for x in pat)
assert len(code)==44
d = tempfile.mkdtemp()
subprocess.check_call(['gcc','-O2','-o',d+'/v',os.path.join(ROOT,'tests','dl_values.c')])
def imm(limit):
    a,b = subprocess.run([d+'/v',str(limit)],capture_output=True,text=True).stdout.split()
    return bytes.fromhex(a), bytes.fromhex(b)
def patched(limit, mutate=None):
    c = bytearray(code)
    if limit is not None:
        a,b = imm(limit)
        if mutate=='swap': a,b = b,a
        c[0x19:0x1D] = a; c[0x28:0x2C] = b
    return bytes(c) + bytes([0xEB,0x00])        # jmp +0 -> falls into 'done' (also the jns target)
# scenario: (name, limit or None=vanilla, expected limit L)
SCEN = [('vanilla (unpatched game)',None,99999),('limit 999999 (default)',999999,999999),
        ('limit 500000',500000,500000),('limit 99999 (must equal vanilla)',99999,99999),('limit 10000 (minimum)',10000,10000)]
if len(sys.argv)>1 and sys.argv[1]=='--mutate': SCEN=[('MUTATED: immediates swapped',999999,999999)]
CASES = [('has the bit',0x0800,0x00,'L'),('no bit',0x0000,0x00,'B'),('forced by the move (bit 7)',0x0000,0x80,'L'),
         ('bit + forced',0x0800,0x80,'L'),('bit among other bits',0x1FFF,0x00,'L'),('other bits, not 0x800',0x07FF,0x00,'B')]
rows=[]; asm = '''.intel_syntax noprefix
.set OUTV,0x0A000000
.set STAT,0x0A000100
.set ATTR,0x0A000900
.set RES,0x0A000A00
.section .mydata,"aw",@progbits
.space 0x1000
.text
.globl _start
_start:
    xor edi, edi
'''
bodies=''
for k,(sname,limit,L) in enumerate(SCEN):
    for cname,status,al,kind in CASES:
        exp = L if kind=='L' else 9999
        rows.append((sname,cname,exp))
        asm += f'''    mov word ptr [STAT+0x6BE], 0x{status:X}
    mov byte ptr [ATTR+0x20], 0x{al:X}
    mov ebx, offset STAT
    mov esi, offset ATTR
    call snip{k}
    mov dl, 0x50
    cmp dword ptr [OUTV], {exp}
    je ok{len(rows)}
    mov dl, 0x46
ok{len(rows)}:
    mov [RES+edi], dl
    inc edi
'''
    bodies += f'snip{k}:\n' + '\n'.join('    .byte '+','.join('0x%02X'%x for x in patched(limit,'swap' if SCEN[0][0].startswith('MUT') else None)[i:i+16]) for i in range(0,46,16)) + '\n    mov dword ptr [OUTV], ebx\n    ret\n'
asm += '''    mov byte ptr [RES+edi], 10
    inc edi
    mov eax, 4
    mov ebx, 1
    mov ecx, 0x0A000A00
    mov edx, edi
    int 0x80
    mov eax, 1
    xor ebx, ebx
    int 0x80
''' + bodies
open(d+'/h.s','w').write(asm)
subprocess.check_call(['as','--32','-o',d+'/h.o',d+'/h.s'])
subprocess.check_call(['ld','-m','elf_i386','--section-start=.mydata=0x0A000000','-o',d+'/h',d+'/h.o'])
res = subprocess.run([d+'/h'],capture_output=True,text=True).stdout.strip()
ok = len(res)==len(rows)
for i,(s,c,e) in enumerate(rows):
    good = i<len(res) and res[i]=='P'; ok &= good
    print('PASS' if good else 'FAIL', f'[{s}]', c, '->', e)
print('\nALL PASS' if ok else '\nSOME FAILED')

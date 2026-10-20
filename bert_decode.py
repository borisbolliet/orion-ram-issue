import struct, uuid, sys

data = open(sys.argv[1], 'rb').read()

SECT = {
 'dc3ea0b0-a144-4797-b95b-53fa242b6e1d': 'IA32/X64 Processor Error',
 'a5bc1114-6f64-4ede-b863-3e83ed7c83b1': 'Platform Memory Error',
 '9876ccad-47b4-4bdb-b65e-16f193c4f3db': 'Processor Generic Error',
 'd995e954-bbc1-430f-ad91-b44dcb3c6f35': 'PCIe Error',
 '81212a96-09ed-4996-9471-8d729c8e69ed': 'Firmware Error Record',
 '2dce8bb1-bdd7-450e-b9ad-9cf4ebd4f890': 'IA32/X64 MCA (generic)',
}
CHECK = {
 'a55701f5-e3ef-43de-ac72-249b573fad2c': 'Cache Check',
 'fc06b535-5e1f-4562-9f25-0a3b9adb63c3': 'TLB Check',
 '1cf3f8b3-c5b1-49a2-aa59-5eef92ffa63c': 'Bus Check',
 '48ab7f57-dc34-4f6c-a7d3-b0b5b0a74314': 'Micro-Architectural Check',
}
SEV = {0: 'Recoverable', 1: 'FATAL', 2: 'Corrected', 3: 'None'}
CTX = {0:'Unclassified',1:'MSR Registers',2:'32-bit x86 context',3:'64-bit x86 context',
       4:'FXSAVE',5:'32-bit debug regs',6:'64-bit debug regs',7:'Memory-mapped regs'}
TXN = {0:'Instruction', 1:'Data Access', 2:'Generic'}
OPN = {0:'Generic error',1:'Generic read',2:'Generic write',3:'Data read',4:'Data write',
       5:'Instruction fetch',6:'Prefetch',7:'Eviction',8:'Snoop'}

def g(b):  # mixed-endian EFI GUID
    return str(uuid.UUID(bytes_le=b))

bs, rdo, rdl, dl, sev = struct.unpack_from('<IIIII', data, 0)
print(f"Generic Error Status Block")
print(f"  block_status   : 0x{bs:08x}")
print(f"  data_length    : {dl} bytes")
print(f"  error_severity : {sev} ({SEV.get(sev,'?')})")

off = 20
n = 0
LIMIT = int(sys.argv[2]) if len(sys.argv) > 2 else 8   # entries to print; 0 = all
while off + 72 <= 20 + dl and (LIMIT == 0 or n < LIMIT):
    st, esev, rev, vb, fl, edl = struct.unpack_from('<16sIHBBI', data, off)
    fru_id = data[off+28:off+44]
    fru_txt = data[off+44:off+64].split(b'\0')[0].decode('ascii','replace')
    ts = struct.unpack_from('<Q', data, off+64)[0]
    st_guid = g(st)
    if edl == 0 or edl > dl: break
    n += 1
    print(f"\n--- Error Data Entry {n} ---")
    print(f"  section type : {SECT.get(st_guid, st_guid)}")
    print(f"  severity     : {esev} ({SEV.get(esev,'?')})")
    print(f"  flags        : 0x{fl:02x} ({'primary ' if fl&1 else ''}{'containment-warning ' if fl&2 else ''}{'reset ' if fl&4 else ''}{'threshold-exceeded ' if fl&8 else ''}{'resource-not-avail ' if fl&16 else ''}{'latent ' if fl&32 else ''})")
    print(f"  FRU text     : {fru_txt!r}")
    print(f"  data length  : {edl} bytes  (revision 0x{rev:04x}, timestamp raw 0x{ts:016x})")

    sec = data[off+72: off+72+edl]
    if st_guid == 'dc3ea0b0-a144-4797-b95b-53fa242b6e1d':
        v = struct.unpack_from('<Q', sec, 0)[0]
        n_err = (v >> 2) & 0x3f
        n_ctx = (v >> 8) & 0x3f
        apic = struct.unpack_from('<Q', sec, 8)[0]
        cpuid = struct.unpack_from('<I', sec, 16)[0]
        fam = ((cpuid>>8)&0xf) + ((cpuid>>20)&0xff)
        mod = ((cpuid>>4)&0xf) | (((cpuid>>16)&0xf)<<4)
        stp = cpuid & 0xf
        print(f"  validation   : 0x{v:x} -> {n_err} error-info struct(s), {n_ctx} context struct(s)")
        print(f"  local APIC id: {apic}")
        print(f"  CPUID eax    : 0x{cpuid:08x} -> family 0x{fam:x} model 0x{mod:x} stepping {stp}")
        p = 64
        for i in range(n_err):
            et, evb, ci, tgt, req, rsp, ip = struct.unpack_from('<16sQQQQQQ', sec, p)
            et_guid = g(et)
            print(f"\n  [Processor Error Info {i+1}] {CHECK.get(et_guid, et_guid)}")
            print(f"     valid bits : 0x{evb:x} (check_info={'Y' if evb&1 else 'n'} target={'Y' if evb&2 else 'n'} requestor={'Y' if evb&4 else 'n'} responder={'Y' if evb&8 else 'n'} ip={'Y' if evb&16 else 'n'})")
            print(f"     check_info : 0x{ci:016x}")
            if et_guid in ('a55701f5-e3ef-43de-ac72-249b573fad2c','fc06b535-5e1f-4562-9f25-0a3b9adb63c3'):
                vbits = ci & 0xff
                parts=[]
                if vbits & 1: parts.append(f"txn={TXN.get((ci>>8)&3,'?')}")
                if vbits & 2: parts.append(f"op={OPN.get((ci>>10)&0xf,'?')}")
                if vbits & 4: parts.append(f"level={(ci>>14)&7}")
                if vbits & 8: parts.append(f"proc-context-corrupt={(ci>>17)&1}")
                if vbits & 16: parts.append(f"UNCORRECTED={(ci>>18)&1}")
                if vbits & 32: parts.append(f"precise-ip={(ci>>19)&1}")
                if vbits & 64: parts.append(f"restartable-ip={(ci>>20)&1}")
                if vbits & 128: parts.append(f"overflow={(ci>>21)&1}")
                print(f"     decoded    : " + ", ".join(parts))
            if evb & 16: print(f"     instr ptr  : 0x{ip:016x}")
            if evb & 2: print(f"     target addr: 0x{tgt:016x}")
            p += 64
        for i in range(n_ctx):
            ctype, asize = struct.unpack_from('<HH', sec, p)
            msr, mmreg = struct.unpack_from('<IQ', sec, p+4)
            print(f"\n  [Context Info {i+1}] type={ctype} ({CTX.get(ctype,'?')}) size={asize} msr_base=0x{msr:x}")
            regs = sec[p+16:p+16+asize]
            if ctype == 1 and asize % 8 == 0:
                for j in range(asize//8):
                    val = struct.unpack_from('<Q', regs, j*8)[0]
                    print(f"       MSR 0x{msr+j:08x} = 0x{val:016x}")
            else:
                print("       " + regs[:64].hex())
            p += 16 + asize
    off += 72 + edl

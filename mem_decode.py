import struct, uuid, sys
d=open(sys.argv[1],"rb").read()
MEM="a5bc1114-6f64-4ede-b863-3e83ed7c83b1"
SEV={0:"Recoverable",1:"FATAL",2:"Corrected",3:"None"}
ET={0:"Unknown",1:"No error",2:"Single-bit ECC",3:"Multi-bit ECC",4:"Single-symbol ChipKill ECC",
 5:"Multi-symbol ChipKill ECC",6:"Master abort",7:"Target abort",8:"Parity error",9:"Watchdog timeout",
 10:"Invalid address",11:"Mirror Broken",12:"Memory sparing",13:"Scrub corrected error",
 14:"Scrub uncorrected error",15:"Physical memory map-out event"}
F=["ErrorStatus","PhysicalAddress","PhysAddrMask","Node","Card","Module","Bank","Device","Row","Column",
   "BitPosition","RequestorId","ResponderId","TargetId","ErrorType","RankNumber","CardHandle","ModuleHandle",
   "ExtendedRow","BankGroup","BankAddress","ChipID"]
bs,rdo,rdl,dl,sev=struct.unpack_from("<IIIII",d,0)
off=20;n=0
while off+72<=20+dl:
    st,esev,rev,vb,fl,edl=struct.unpack_from("<16sIHBBI",d,off)
    if edl==0 or edl>dl: break
    n+=1
    if str(uuid.UUID(bytes_le=st))==MEM:
        fru=d[off+44:off+64].split(b"\0")[0].decode("ascii","replace")
        s=d[off+72:off+72+edl]
        v,es,pa,pam=struct.unpack_from("<QQQQ",s,0)
        node,card,mod,bank,dev,row,col,bitpos=struct.unpack_from("<8H",s,32)
        etype,ext,rank,chandle,mhandle=struct.unpack_from("<BBHHH",s,72)
        et_s="%d (%s)"%(etype,ET.get(etype,"?"))
        print("=== entry %d: %s  FRU=%r  (valid=0x%x) ==="%(n,SEV.get(esev),fru,v))
        print("    valid fields:", ", ".join(F[i] for i in range(len(F)) if v>>i&1))
        def p(bit,label,val):
            if v>>bit&1: print("    %-16s: %s"%(label,val))
        p(0,"ErrorStatus","0x%016x"%es); p(1,"PhysicalAddress","0x%016x"%pa)
        p(2,"PhysAddrMask","0x%016x"%pam); p(3,"Node",node); p(4,"Card",card)
        p(5,"Module",mod); p(6,"Bank",bank); p(7,"Device",dev); p(8,"Row",row)
        p(9,"Column",col); p(10,"BitPosition",bitpos); p(14,"ErrorType",et_s)
        p(15,"RankNumber",rank); p(16,"CardHandle","0x%04x"%chandle); p(17,"ModuleHandle","0x%04x"%mhandle)
        print()
    off+=72+edl

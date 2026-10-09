"""Extract a time window from a measured VCD, retaining real initial channel levels."""
import json
import re
from pathlib import Path
from hashlib import sha256

folder=Path(__file__).resolve().parents[1]/"docs/captures"
def crop(src, name, start, end):
    text=(folder/src).read_text(); header=text[:text.index("$enddefinitions $end")+len("$enddefinitions $end")]
    state={}; events=[]; t=0
    for line in text.splitlines():
        if line.startswith("#"):t=int(line[1:])
        elif len(line)==2 and line[0] in "01":
            if t<=start:state[line[1]]=line[0]
            elif t<=end:events.append((t-start,line))
    out=header+"\n#0\n"+"\n".join(v+k for k,v in sorted(state.items()))+"\n"
    for t,line in events:out+=f"#{t}\n{line}\n"
    out+=f"#{end-start}\n"
    (folder/(name+".vcd")).write_text(out)
    (folder/(name+".origin.json")).write_text(json.dumps({"source":src,"sha256":sha256(text.encode()).hexdigest(),"start_ns":start,"end_ns":end,"transform":"exact window; initial levels from source; timestamps shifted to zero"},indent=2))
def annotations(name):
    return [(int(a)*500,int(b)*500,msg) for a,b,msg in re.findall(r'(\d+)-(\d+) i2c-1: ([^\n]+)',(folder/name).read_text())]
n=annotations("normal-i2c.txt"); f=annotations("address-nack-i2c.txt")
first=n[0][0]; id_end=next(b for a,b,msg in n[8:] if msg=="Stop")
crop("normal.vcd","normal-id",first-50000,id_end+50000)
i=next(i for i,x in enumerate(n) if x[2]=="Data write: 3B")
start=next(a for a,b,msg in reversed(n[:i]) if msg=="Start")
last=next(b for a,b,msg in n[i+5:] if msg=="Stop")
crop("normal.vcd","normal-data",start-50000,last+50000)
i=next(i for i,x in enumerate(f) if x[2]=="Address write: 69")
start=next(a for a,b,msg in reversed(f[:i]) if msg=="Start")
end=next(b for a,b,msg in f[i+1:] if msg=="Stop")
crop("address-nack.vcd","fault-address",start-50000,end+50000)
crop('normal.vcd','uart-boot',3_400_000,4_200_000)
crop('normal.vcd','adc-period',90_000_000,315_000_000)
print("PASS extracted normal-id, normal-data and fault-address without invented transitions")

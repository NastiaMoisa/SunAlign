"""Plot an annotated event only from a successful real VCD analysis JSON."""
import json,argparse,pathlib
import matplotlib.pyplot as plt
p=argparse.ArgumentParser();p.add_argument('analysis');p.add_argument('--output',required=True);a=p.parse_args()
d=json.loads(pathlib.Path(a.analysis).read_text());pairs=d['button_pairs']
if not pairs:raise SystemExit('No measured ZERO event to annotate')
pair=next((x for x in pairs if x['during_long']),pairs[0]);start=pair['edge_ns'];end=pair['accept_ns']
fig,ax=plt.subplots(figsize=(10,3.7));ms=lambda t:(t-start)/1e6
ax.scatter([0],[2],marker='v',s=85,color='#26436b')
for lo,hi in d['long_intervals_ns']:
 if hi>=start-5e6 and lo<=end+5e6:ax.broken_barh([(ms(lo),(hi-lo)/1e6)],(.5,.5),facecolors='#df8e35')
ax.scatter([ms(end)],[0],marker='^',s=85,color='#24714e')
ax.annotate('',xy=(ms(end),2.35),xytext=(0,2.35),arrowprops=dict(arrowstyle='<->'))
ax.text(ms(end)/2,2.43,f"Measured edge-to-accept: {pair['latency_us']/1000:.3f} ms",ha='center')
ax.set(yticks=[2,.75,0],yticklabels=['PB0 falling edge','PA6 long operation','PA7 transition'],xlabel='Time relative to physical input edge, ms',xlim=(-5,ms(end)+5),ylim=(-.4,2.8))
ax.set_title('SunAlign: measured events from original Wokwi VCD');ax.grid(axis='x',alpha=.2)
fig.tight_layout();fig.savefig(a.output,dpi=180);plt.close(fig)

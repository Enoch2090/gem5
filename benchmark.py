import os
import re

from collections import OrderedDict

def conv_num(val):
    if type(val) is list:
        return [conv_num(_v) for _v in val]
    try:
        return int(val)
    except ValueError:
        try:
            return float(val)
        except ValueError:
            return val


with open('./m5out/stats.txt', 'r') as f:
    stats = [
        re.split(
            r'\s+',
            re.sub(
                r'\s*# [\S\s]*$', '', _x
            )
        ) for _x in f.readlines()[1:-1] \
            if not '---------' in _x and len(_x) > 3
    ]

stats_dict = OrderedDict(
    {
        _s[0]: conv_num(_s[1]) \
            if len(_s)==2 else _s[1::] \
                for _s in stats
    }
)

ipc = stats_dict["system.cpu.ipc"]
incorr = stats_dict["system.cpu.branchPred.condIncorrect"]
totbr = stats_dict["system.cpu.branchPred.condPredicted"]
mispred = stats_dict["system.cpu.commit.branchMispredicts"]
ninst = stats_dict["system.cpu.thread_0.numInsts"]

print(f'IPC={ipc:.4f}')

print(f'acc={(1 - incorr / totbr) :.4f}')

print(f'MPKI={(mispred / ninst * 1000) :.4f}')


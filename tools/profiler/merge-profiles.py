






import json
import re
import sys

def MergeProfiles(files):
    threads = []
    fileData = []
    symTable = dict()
    meta = None
    libs = None
    minStartTime = None

    for fname in files:
        match = re.match('profile_([0-9]+)_(.+)\.sym', fname)
        if match is None:
            raise Exception("Filename '" + fname + "' doesn't match expected pattern")
        pid = match.groups(0)[0]
        pname = match.groups(0)[1]

        fp = open(fname, "r")
        fileData = json.load(fp)
        fp.close()

        if meta is None:
            meta = fileData['profileJSON']['meta'].copy()
            libs = fileData['profileJSON']['libs']
            minStartTime = meta['startTime']
        else:
            minStartTime = min(minStartTime, fileData['profileJSON']['meta']['startTime'])
            meta['startTime'] = minStartTime

        for thread in fileData['profileJSON']['threads']:
            thread['name'] = thread['name'] + " (" + pname + ":" + pid + ")"
            threads.append(thread)

            
            
            
            pidStr = "%05d" % (int(pid))

            thread['startTime'] = fileData['profileJSON']['meta']['startTime']
            samples = thread['samples']
            for sample in thread['samples']:
                for frame in sample['frames']:
                    if "location" in frame and frame['location'][0:2] == '0x':
                        frame['location'] = pidStr + frame['location']

        filesyms = fileData['symbolicationTable']
        for sym in filesyms.keys():
            symTable[pidStr + sym] = filesyms[sym]

    
    
    for thread in threads:
        delta = thread['startTime'] - minStartTime
        for sample in thread['samples']:
            if "time" in sample:
                sample['time'] += delta

    result = dict()
    result['profileJSON'] = dict()
    result['profileJSON']['meta'] = meta
    result['profileJSON']['libs'] = libs
    result['profileJSON']['threads'] = threads
    result['symbolicationTable'] = symTable
    result['format'] = "profileJSONWithSymbolicationTable,1"

    json.dump(result, sys.stdout)


if len(sys.argv) > 1:
    MergeProfiles(sys.argv[1:])
    sys.exit(0)

print "Usage: merge-profile.py profile_<pid1>_<pname1>.sym profile_<pid2>_<pname2>.sym > merged.sym"




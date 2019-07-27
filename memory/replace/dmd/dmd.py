





'''This script analyzes a JSON file emitted by DMD.'''

from __future__ import print_function, division

import argparse
import collections
import gzip
import json
import os
import platform
import re
import shutil
import sys
import tempfile


outputVersion = 3





allocatorFns = [
    'replace_malloc',
    'replace_calloc',
    'replace_realloc',
    'replace_memalign',
    'replace_posix_memalign',
    'moz_xmalloc',
    'moz_xcalloc',
    'moz_xrealloc',
    'operator new(',
    'operator new[](',
    'g_malloc',
    'g_slice_alloc',
    'callocCanGC',
    'reallocCanGC',
    'vpx_malloc',
    'vpx_calloc',
    'vpx_realloc',
    'vpx_memalign',
    'js_malloc',
    'js_calloc',
    'js_realloc',
    'pod_malloc',
    'pod_calloc',
    'pod_realloc',
    
    
    
    '???',
]

class Record(object):
    '''A record is an aggregation of heap blocks that have identical stack
    traces. It can also be used to represent the difference between two
    records.'''

    def __init__(self):
        self.numBlocks = 0
        self.reqSize = 0
        self.slopSize = 0
        self.usableSize = 0
        self.isSampled = False
        self.allocatedAtDesc = None
        self.reportedAtDescs = []
        self.usableSizes = collections.defaultdict(int)

    def isZero(self, args):
        return self.numBlocks == 0 and \
               self.reqSize == 0 and \
               self.slopSize == 0 and \
               self.usableSize == 0 and \
               len(self.usableSizes) == 0

    def negate(self):
        self.numBlocks = -self.numBlocks
        self.reqSize = -self.reqSize
        self.slopSize = -self.slopSize
        self.usableSize = -self.usableSize

        negatedUsableSizes = collections.defaultdict(int)
        for (usableSize, isSampled), count in self.usableSizes.items():
            negatedUsableSizes[(-usableSize, isSampled)] = count
        self.usableSizes = negatedUsableSizes

    def subtract(self, r):
        
        
        assert self.allocatedAtDesc == r.allocatedAtDesc
        assert self.reportedAtDescs == r.reportedAtDescs

        self.numBlocks -= r.numBlocks
        self.reqSize -= r.reqSize
        self.slopSize -= r.slopSize
        self.usableSize -= r.usableSize
        self.isSampled = self.isSampled or r.isSampled

        usableSizes1 = self.usableSizes
        usableSizes2 = r.usableSizes
        usableSizes3 = collections.defaultdict(int)
        for usableSize, isSampled in usableSizes1:
            counts1 = usableSizes1[usableSize, isSampled]
            if (usableSize, isSampled) in usableSizes2:
                counts2 = usableSizes2[usableSize, isSampled]
                del usableSizes2[usableSize, isSampled]
                counts3 = counts1 - counts2
                if counts3 != 0:
                    if counts3 < 0:
                        usableSize = -usableSize
                        counts3 = -counts3
                    usableSizes3[usableSize, isSampled] = counts3
            else:
                usableSizes3[usableSize, isSampled] = counts1

        for usableSize, isSampled in usableSizes2:
            usableSizes3[-usableSize, isSampled] = \
                usableSizes2[usableSize, isSampled]

        self.usableSizes = usableSizes3

    @staticmethod
    def cmpByIsSampled(r1, r2):
        
        return cmp(r2.isSampled, r1.isSampled)

    @staticmethod
    def cmpByUsableSize(r1, r2):
        
        return cmp(abs(r1.usableSize), abs(r2.usableSize)) or \
               Record.cmpByReqSize(r1, r2)

    @staticmethod
    def cmpByReqSize(r1, r2):
        
        return cmp(abs(r1.reqSize), abs(r2.reqSize)) or \
               Record.cmpByIsSampled(r1, r2)

    @staticmethod
    def cmpBySlopSize(r1, r2):
        
        return cmp(abs(r1.slopSize), abs(r2.slopSize)) or \
               Record.cmpByIsSampled(r1, r2)


sortByChoices = {
    'usable': Record.cmpByUsableSize,   
    'req':    Record.cmpByReqSize,
    'slop':   Record.cmpBySlopSize,
}


def parseCommandLine():
    
    def range_1_24(string):
        value = int(string)
        if value < 1 or value > 24:
            msg = '{:s} is not in the range 1..24'.format(string)
            raise argparse.ArgumentTypeError(msg)
        return value

    description = '''
Analyze heap data produced by DMD.
If one file is specified, analyze it; if two files are specified, analyze the
difference.
Input files can be gzipped.
Write to stdout unless -o/--output is specified.
Stack traces are fixed to show function names, filenames and line numbers
unless --no-fix-stacks is specified; stack fixing modifies the original file
and may take some time. If specified, the BREAKPAD_SYMBOLS_PATH environment
variable is used to find breakpad symbols for stack fixing.
'''
    p = argparse.ArgumentParser(description=description)

    p.add_argument('-o', '--output', type=argparse.FileType('w'),
                   help='output file; stdout if unspecified')

    p.add_argument('-f', '--max-frames', type=range_1_24,
                   help='maximum number of frames to consider in each trace')

    p.add_argument('-s', '--sort-by', choices=sortByChoices.keys(),
                   default=sortByChoices.keys()[0],
                   help='sort the records by a particular metric')

    p.add_argument('-a', '--ignore-alloc-fns', action='store_true',
                   help='ignore allocation functions at the start of traces')

    p.add_argument('--no-fix-stacks', action='store_true',
                   help='do not fix stacks')

    p.add_argument('--filter-stacks-for-testing', action='store_true',
                   help='filter stack traces; only useful for testing purposes')

    p.add_argument('input_file',
                   help='a file produced by DMD')

    p.add_argument('input_file2', nargs='?',
                   help='a file produced by DMD; if present, it is diff\'d with input_file')

    return p.parse_args(sys.argv[1:])




def fixStackTraces(inputFilename, isZipped, opener):
    
    
    sys.path.append(os.path.dirname(__file__))

    bpsyms = os.environ.get('BREAKPAD_SYMBOLS_PATH', None)
    sysname = platform.system()
    if bpsyms and os.path.exists(bpsyms):
        import fix_stack_using_bpsyms as fixModule
        fix = lambda line: fixModule.fixSymbols(line, bpsyms)
    elif sysname == 'Linux':
        import fix_linux_stack as fixModule
        fix = lambda line: fixModule.fixSymbols(line)
    elif sysname == 'Darwin':
        import fix_macosx_stack as fixModule
        fix = lambda line: fixModule.fixSymbols(line)
    else:
        fix = None  

    if fix:
        
        
        tmpFile = tempfile.NamedTemporaryFile(delete=False)

        
        
        
        
        
        
        
        
        
        
        
        tmpFilename = tmpFile.name
        if isZipped:
            tmpFile = gzip.GzipFile(filename='', fileobj=tmpFile)

        with opener(inputFilename, 'rb') as inputFile:
            for line in inputFile:
                tmpFile.write(fix(line))

        tmpFile.close()

        shutil.move(tmpFilename, inputFilename)


def getDigestFromFile(args, inputFile):
    
    isZipped = inputFile.endswith('.gz')
    opener = gzip.open if isZipped else open

    
    if not args.no_fix_stacks:
        fixStackTraces(inputFile, isZipped, opener)

    with opener(inputFile, 'rb') as f:
        j = json.load(f)

    if j['version'] != outputVersion:
        raise Exception("'version' property isn't '{:d}'".format(outputVersion))

    
    invocation = j['invocation']
    dmdEnvVar = invocation['dmdEnvVar']
    mode = invocation['mode']
    sampleBelowSize = invocation['sampleBelowSize']
    blockList = j['blockList']
    traceTable = j['traceTable']
    frameTable = j['frameTable']

    if not mode in ['live', 'dark-matter', 'cumulative']:
        raise Exception("bad 'mode' property: '{:s}'".format(mode))

    heapIsSampled = sampleBelowSize > 1     

    
    if args.ignore_alloc_fns:
        
        escapedAllocatorFns = map(re.escape, allocatorFns)
        fn_re = re.compile('|'.join(escapedAllocatorFns))

        
        for traceKey, frameKeys in traceTable.items():
            numSkippedFrames = 0
            for frameKey in frameKeys:
                frameDesc = frameTable[frameKey]
                if re.search(fn_re, frameDesc):
                    numSkippedFrames += 1
                else:
                    break
            if numSkippedFrames > 0:
                traceTable[traceKey] = frameKeys[numSkippedFrames:]

    
    for traceKey, frameKeys in traceTable.items():
        if len(frameKeys) > args.max_frames:
            traceTable[traceKey] = frameKeys[:args.max_frames]

    def buildTraceDescription(traceTable, frameTable, traceKey):
        frameKeys = traceTable[traceKey]
        fmt = '    #{:02d}{:}'

        if args.filter_stacks_for_testing:
            
            
            
            
            
            
            
            
            for frameKey in frameKeys:
                frameDesc = frameTable[frameKey]
                if 'DMD.cpp' in frameDesc or 'dmd.cpp' in frameDesc:
                    return [fmt.format(1, ': ... DMD.cpp ...')]

        
        
        desc = []
        for n, frameKey in enumerate(traceTable[traceKey], start=1):
            desc.append(fmt.format(n, frameTable[frameKey][3:]))
        return desc

    
    

    if mode in ['live', 'cumulative']:
        liveOrCumulativeRecords = collections.defaultdict(Record)
    elif mode == 'dark-matter':
        unreportedRecords    = collections.defaultdict(Record)
        onceReportedRecords  = collections.defaultdict(Record)
        twiceReportedRecords = collections.defaultdict(Record)

    heapUsableSize = 0
    heapBlocks = 0

    for block in blockList:
        
        
        
        
        
        
        
        
        
        
        
        
        
        
        
        
        
        def makeRecordKeyPart(traceKey):
            return str(map(lambda frameKey: frameTable[frameKey],
                           traceTable[traceKey]))

        allocatedAtTraceKey = block['alloc']
        if mode in ['live', 'cumulative']:
            recordKey = makeRecordKeyPart(allocatedAtTraceKey)
            records = liveOrCumulativeRecords
        elif mode == 'dark-matter':
            recordKey = makeRecordKeyPart(allocatedAtTraceKey)
            if 'reps' in block:
                reportedAtTraceKeys = block['reps']
                for reportedAtTraceKey in reportedAtTraceKeys:
                    recordKey += makeRecordKeyPart(reportedAtTraceKey)
                if len(reportedAtTraceKeys) == 1:
                    records = onceReportedRecords
                else:
                    records = twiceReportedRecords
            else:
                records = unreportedRecords

        record = records[recordKey]

        if 'req' in block:
            
            reqSize = block['req']
            slopSize = block.get('slop', 0)
            isSampled = False
        else:
            
            reqSize = sampleBelowSize
            if 'slop' in block:
                raise Exception("'slop' property in sampled block'")
            slopSize = 0
            isSampled = True

        usableSize = reqSize + slopSize
        heapUsableSize += usableSize
        heapBlocks += 1

        record.numBlocks  += 1
        record.reqSize    += reqSize
        record.slopSize   += slopSize
        record.usableSize += usableSize
        record.isSampled   = record.isSampled or isSampled
        if record.allocatedAtDesc == None:
            record.allocatedAtDesc = \
                buildTraceDescription(traceTable, frameTable,
                                      allocatedAtTraceKey)

        if mode in ['live', 'cumulative']:
            pass
        elif mode == 'dark-matter':
            if 'reps' in block and record.reportedAtDescs == []:
                f = lambda k: buildTraceDescription(traceTable, frameTable, k)
                record.reportedAtDescs = map(f, reportedAtTraceKeys)
        record.usableSizes[(usableSize, isSampled)] += 1

    
    digest = {}
    digest['dmdEnvVar'] = dmdEnvVar
    digest['mode'] = mode
    digest['sampleBelowSize'] = sampleBelowSize
    digest['heapUsableSize'] = heapUsableSize
    digest['heapBlocks'] = heapBlocks
    digest['heapIsSampled'] = heapIsSampled
    if mode in ['live', 'cumulative']:
        digest['liveOrCumulativeRecords'] = liveOrCumulativeRecords
    elif mode == 'dark-matter':
        digest['unreportedRecords'] = unreportedRecords
        digest['onceReportedRecords'] = onceReportedRecords
        digest['twiceReportedRecords'] = twiceReportedRecords
    return digest


def diffRecords(args, records1, records2):
    records3 = {}

    
    for k in records1:
        r1 = records1[k]
        if k in records2:
            
            r2 = records2[k]
            del records2[k]
            r2.subtract(r1)
            if not r2.isZero(args):
                records3[k] = r2
        else:
            
            r1.negate()
            records3[k] = r1

    for k in records2:
        
        records3[k] = records2[k]

    return records3


def diffDigests(args, d1, d2):
    if (d1['mode'] != d2['mode']):
        raise Exception("the input files have different 'mode' properties")

    d3 = {}
    d3['dmdEnvVar'] = (d1['dmdEnvVar'], d2['dmdEnvVar'])
    d3['mode'] = d1['mode']
    d3['sampleBelowSize'] = (d1['sampleBelowSize'], d2['sampleBelowSize'])
    d3['heapUsableSize'] = d2['heapUsableSize'] - d1['heapUsableSize']
    d3['heapBlocks']     = d2['heapBlocks']     - d1['heapBlocks']
    d3['heapIsSampled']  = d2['heapIsSampled'] or d1['heapIsSampled']
    if d1['mode'] in ['live', 'cumulative']:
        d3['liveOrCumulativeRecords'] = \
            diffRecords(args, d1['liveOrCumulativeRecords'],
                              d2['liveOrCumulativeRecords'])
    elif d1['mode'] == 'dark-matter':
        d3['unreportedRecords']    = diffRecords(args, d1['unreportedRecords'],
                                                       d2['unreportedRecords'])
        d3['onceReportedRecords']  = diffRecords(args, d1['onceReportedRecords'],
                                                       d2['onceReportedRecords'])
        d3['twiceReportedRecords'] = diffRecords(args, d1['twiceReportedRecords'],
                                                       d2['twiceReportedRecords'])
    return d3


def printDigest(args, digest):
    dmdEnvVar       = digest['dmdEnvVar']
    mode            = digest['mode']
    sampleBelowSize = digest['sampleBelowSize']
    heapUsableSize  = digest['heapUsableSize']
    heapIsSampled   = digest['heapIsSampled']
    heapBlocks      = digest['heapBlocks']
    if mode in ['live', 'cumulative']:
        liveOrCumulativeRecords = digest['liveOrCumulativeRecords']
    elif mode == 'dark-matter':
        unreportedRecords    = digest['unreportedRecords']
        onceReportedRecords  = digest['onceReportedRecords']
        twiceReportedRecords = digest['twiceReportedRecords']

    separator = '#' + '-' * 65 + '\n'

    def number(n, isSampled):
        '''Format a number, with comma as a separator and a '~' prefix if it's
        sampled.'''
        return '{:}{:,d}'.format('~' if isSampled else '', n)

    def perc(m, n):
        return 0 if n == 0 else (100 * m / n)

    def plural(n):
        return '' if n == 1 else 's'

    
    def out(*arguments, **kwargs):
        print(*arguments, file=args.output, **kwargs)

    def printStack(traceDesc):
        for frameDesc in traceDesc:
            out(frameDesc)

    def printRecords(recordKind, records, heapUsableSize):
        RecordKind = recordKind.capitalize()
        out(separator)
        numRecords = len(records)
        cmpRecords = sortByChoices[args.sort_by]
        sortedRecords = sorted(records.values(), cmp=cmpRecords, reverse=True)
        kindBlocks = 0
        kindUsableSize = 0
        maxRecord = 1000

        
        for record in sortedRecords:
            kindBlocks     += record.numBlocks
            kindUsableSize += record.usableSize

        
        if numRecords == 0:
            out('# no {:} heap blocks\n'.format(recordKind))

        kindCumulativeUsableSize = 0
        for i, record in enumerate(sortedRecords, start=1):
            
            if i == maxRecord:
                out('# {:}: stopping after {:,d} heap block records\n'.
                    format(RecordKind, i))
                break

            kindCumulativeUsableSize += record.usableSize

            isSampled = record.isSampled

            out(RecordKind + ' {')
            out('  {:} block{:} in heap block record {:,d} of {:,d}'.
                format(number(record.numBlocks, isSampled),
                       plural(record.numBlocks), i, numRecords))
            out('  {:} bytes ({:} requested / {:} slop)'.
                format(number(record.usableSize, isSampled),
                       number(record.reqSize, isSampled),
                       number(record.slopSize, isSampled)))

            abscmp = lambda ((usableSize1, _1a), _1b), \
                            ((usableSize2, _2a), _2b): \
                            cmp(abs(usableSize1), abs(usableSize2))
            usableSizes = sorted(record.usableSizes.items(), cmp=abscmp,
                                 reverse=True)

            hasSingleBlock = len(usableSizes) == 1 and usableSizes[0][1] == 1

            if not hasSingleBlock:
                out('  Individual block sizes: ', end='')
                if len(usableSizes) == 0:
                    out('(no change)', end='')
                else:
                    isFirst = True
                    for (usableSize, isSampled), count in usableSizes:
                        if not isFirst:
                            out('; ', end='')
                        out('{:}'.format(number(usableSize, isSampled)), end='')
                        if count > 1:
                            out(' x {:,d}'.format(count), end='')
                        isFirst = False
                out()

            out('  {:4.2f}% of the heap ({:4.2f}% cumulative)'.
                format(perc(record.usableSize, heapUsableSize),
                       perc(kindCumulativeUsableSize, heapUsableSize)))
            if mode in ['live', 'cumulative']:
                pass
            elif mode == 'dark-matter':
                out('  {:4.2f}% of {:} ({:4.2f}% cumulative)'.
                    format(perc(record.usableSize, kindUsableSize),
                           recordKind,
                           perc(kindCumulativeUsableSize, kindUsableSize)))
            out('  Allocated at {')
            printStack(record.allocatedAtDesc)
            out('  }')
            if mode in ['live', 'cumulative']:
                pass
            elif mode == 'dark-matter':
                for n, reportedAtDesc in enumerate(record.reportedAtDescs):
                    again = 'again ' if n > 0 else ''
                    out('  Reported {:}at {{'.format(again))
                    printStack(reportedAtDesc)
                    out('  }')
            out('}\n')

        return (kindUsableSize, kindBlocks)


    def printInvocation(n, dmdEnvVar, sampleBelowSize):
        out('Invocation{:} {{'.format(n))
        if dmdEnvVar == None:
            out('  $DMD is undefined')
        else:
            out('  $DMD = \'' + dmdEnvVar + '\'')
        out('  Sample-below size = ' + str(sampleBelowSize))
        out('}\n')

    
    
    out(separator, end='')
    out('# ' + ' '.join(map(os.path.basename, sys.argv)) + '\n')

    
    if type(dmdEnvVar) is not tuple:
        printInvocation('', dmdEnvVar, sampleBelowSize)
    else:
        printInvocation(' 1', dmdEnvVar[0], sampleBelowSize[0])
        printInvocation(' 2', dmdEnvVar[1], sampleBelowSize[1])

    
    if mode in ['live', 'cumulative']:
        liveOrCumulativeUsableSize, liveOrCumulativeBlocks = \
            printRecords(mode, liveOrCumulativeRecords, heapUsableSize)
    elif mode == 'dark-matter':
        twiceReportedUsableSize, twiceReportedBlocks = \
            printRecords('twice-reported', twiceReportedRecords, heapUsableSize)

        unreportedUsableSize, unreportedBlocks = \
            printRecords('unreported', unreportedRecords, heapUsableSize)

        onceReportedUsableSize, onceReportedBlocks = \
            printRecords('once-reported', onceReportedRecords, heapUsableSize)

    
    out(separator)
    out('Summary {')
    if mode in ['live', 'cumulative']:
        out('  Total: {:} bytes in {:} blocks'.
            format(number(liveOrCumulativeUsableSize, heapIsSampled),
                   number(liveOrCumulativeBlocks, heapIsSampled)))
    elif mode == 'dark-matter':
        fmt = '  {:15} {:>12} bytes ({:6.2f}%) in {:>7} blocks ({:6.2f}%)'
        out(fmt.
            format('Total:',
                   number(heapUsableSize, heapIsSampled),
                   100,
                   number(heapBlocks, heapIsSampled),
                   100))
        out(fmt.
            format('Unreported:',
                   number(unreportedUsableSize, heapIsSampled),
                   perc(unreportedUsableSize, heapUsableSize),
                   number(unreportedBlocks, heapIsSampled),
                   perc(unreportedBlocks, heapBlocks)))
        out(fmt.
            format('Once-reported:',
                   number(onceReportedUsableSize, heapIsSampled),
                   perc(onceReportedUsableSize, heapUsableSize),
                   number(onceReportedBlocks, heapIsSampled),
                   perc(onceReportedBlocks, heapBlocks)))
        out(fmt.
            format('Twice-reported:',
                   number(twiceReportedUsableSize, heapIsSampled),
                   perc(twiceReportedUsableSize, heapUsableSize),
                   number(twiceReportedBlocks, heapIsSampled),
                   perc(twiceReportedBlocks, heapBlocks)))
    out('}\n')


def main():
    args = parseCommandLine()
    digest = getDigestFromFile(args, args.input_file)
    if args.input_file2:
        digest2 = getDigestFromFile(args, args.input_file2)
        digest = diffDigests(args, digest, digest2)
    printDigest(args, digest)


if __name__ == '__main__':
    main()


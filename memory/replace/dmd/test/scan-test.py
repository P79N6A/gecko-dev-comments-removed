





'''Testing for the JSON file emitted by DMD heap scan mode when running SmokeDMD.'''

from __future__ import print_function, division

import argparse
import gzip
import json
import sys


outputVersion = 4


def parseCommandLine():
    description = '''
Ensure that DMD heap scan mode creates the correct output when run with SmokeDMD.
This is only for testing. Input files can be gzipped.
'''
    p = argparse.ArgumentParser(description=description)

    p.add_argument('--clamp-contents', action='store_true',
                   help='expect that the contents of the JSON input file have had their addresses clamped')

    p.add_argument('input_file',
                   help='a file produced by DMD')

    return p.parse_args(sys.argv[1:])


def checkScanContents(contents, expected):
    if len(contents) != len(expected):
        raise Exception("Expected " + str(len(expected)) + " things in contents but found " + str(len(contents)))

    for i in range(len(expected)):
        if contents[i] != expected[i]:
            raise Exception("Expected to find " + expected[i] + " at offset " + str(i) + " but found " + contents[i])


def main():
    args = parseCommandLine()

    
    isZipped = args.input_file.endswith('.gz')
    opener = gzip.open if isZipped else open

    with opener(args.input_file, 'rb') as f:
        j = json.load(f)

    if j['version'] != outputVersion:
        raise Exception("'version' property isn't '{:d}'".format(outputVersion))

    invocation = j['invocation']

    mode = invocation['mode']
    if mode != 'scan':
        raise Exception("bad 'mode' property: '{:s}'".format(mode))

    sampleBelowSize = invocation['sampleBelowSize']
    if sampleBelowSize != 1:
        raise Exception("Expected sampleBelowSize of 1 but got " + sampleBelowSize)

    blockList = j['blockList']

    if len(blockList) != 1:
        raise Exception("Expected only one block")

    b = blockList[0]

    
    if args.clamp_contents:
        expected = ['0', '0', '0', b['addr'], b['addr']]
    else:
        addr = int(b['addr'], 16)
        expected = ['123', '0', str(format(addr - 1, 'x')), b['addr'],
                    str(format(addr + 1, 'x')), '0']

    checkScanContents(b['contents'], expected)


if __name__ == '__main__':
    main()

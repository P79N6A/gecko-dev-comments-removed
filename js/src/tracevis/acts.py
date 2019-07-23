




states = [
    'exitlast',
    'interpret',
    'monitor',
    'record',
    'compile',
    'execute',
    'native',
]


event_start = 8;

flush_reasons = [
    'B',
    'O',
    'S',
    'G'
]


reasons = [
    'none',
    'abort',
    'inner',
    'doubles',
    'callback',
    'anchor',
    'backoff',
    'cold',
    'record',
    'peers',
    'execute',
    'stabilize',
    'extendFlush',
    'extendMaxBranches',
    'extendStart',
    'extendCold',
    'extendOuter',
    'mismatchExit',
    'oomExit',
    'overflowExit',
    'timeoutExit',
    'deepBailExit',
    'statusExit',
    'otherExit',
    
    'start',
]



speedup_d = {
    'interpret': 1.0,
    'record': 0.95,
    'native': 2.5
}

speedups = [ speedup_d.get(name, 0) for name in states ]

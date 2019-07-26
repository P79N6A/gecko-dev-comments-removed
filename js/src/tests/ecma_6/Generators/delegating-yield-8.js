

function* countdown(n) {
    while (n > 0) {
        yield n;
        yield* countdown(--n);
    }
    return 34;
}

function collect_results(iter) {
    var ret = [];
    var result;
    do {
        result = iter.next();
        ret.push(result);
    } while (!result.done);
    return ret;
}

var expected = [
    
    {value: 3, done: false},
    
    {value: 2, done: false},
    
    {value: 1, done: false},
    
    
    {value: 1, done: false},
    
    {value: 2, done: false},
    
    {value: 1, done: false},
    
    {value: 1, done: false},
    
    {value: 34, done: true}
];

assertDeepEq(expected, collect_results(countdown(3)));

if (typeof reportCompare == "function")
    reportCompare(true, true);

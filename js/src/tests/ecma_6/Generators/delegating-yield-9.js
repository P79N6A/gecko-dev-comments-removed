

function* countdown(n) {
    while (n > 0) {
        yield (yield* countdown(--n));
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
    
    
    
    {value: 34, done: false},
    {value: 34, done: false},
    {value: 34, done: false},
    {value: 34, done: false},
    {value: 34, done: false},
    {value: 34, done: false},
    {value: 34, done: false},
    {value: 34, done: true}, 
];

assertDeepEq(collect_results(countdown(3)), expected);

if (typeof reportCompare == "function")
    reportCompare(true, true);

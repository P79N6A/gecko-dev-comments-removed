

function results(results) {
    var i = 0;
    function iterator() {
        return this;
    }
    function next() {
        return results[i++];
    }
    var ret = { next: next }
    ret[std_iterator] = iterator;
    return ret;
}

function* yield_results(expected) {
    return yield* Proxy(results(expected), {});
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


var expected = [{value: 1}, 13, "foo", {value: 34, done: true}];


assertDeepEq(expected, collect_results(results(expected)));
assertDeepEq(expected, collect_results(yield_results(expected)));

if (typeof reportCompare == "function")
    reportCompare(true, true);

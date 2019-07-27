



var log = [];
function LoggingProxy(target) {
    var h = {
        defineProperty: function (t, id) {
            log.push("define", id);
            return undefined;
        },
        has: function (t, id) {
            log.push("has", id);
            return id in t;
        },
        get: function (t, id) {
            log.push("get", id);
            return t[id];
        },
        set: function (t, id, v) {
            log.push("set", id);
            t[id] = v;
        }
    };
    return new Proxy(target || [], h);
}




LoggingProxy.from = Array.from;
LoggingProxy.from([3, 4, 5]);
assertDeepEq(log, ["define", "0", "define", "1", "define", "2", "set", "length"]);



log = [];
assertDeepEq(Array.from(new LoggingProxy([3, 4, 5])), [3, 4, 5]);
assertDeepEq(log, ["get", Symbol.iterator,
                   "get", "length", "get", "0",
                   "get", "length", "get", "1",
                   "get", "length", "get", "2",
                   "get", "length"]);


log = [];
var arr = [5, 6, 7];
arr[Symbol.iterator] = undefined;
assertDeepEq(Array.from(new LoggingProxy(arr)), [5, 6, 7]);
assertDeepEq(log, ["get", Symbol.iterator,
                   "get", "length", "get", "0", "get", "1", "get", "2"]);

if (typeof reportCompare === 'function')
    reportCompare(0, 0);

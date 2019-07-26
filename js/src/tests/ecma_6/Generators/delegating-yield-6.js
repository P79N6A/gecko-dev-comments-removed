


var log = "";

function collect_results(iter) {
    var ret = [];
    var result;
    do {
        result = iter.next();
        ret.push(result);
    } while (!result.done);
    return ret;
}

function Iter(val, count) {
    function next() {
        return {
            get done() { log += "d"; return count-- == 0; },
            get value() { log += "v"; return val; }
        }
    }

    this.next = next;
}

function* delegate(iter) { return yield* iter; }

var inner = new Iter(42, 5);
var outer = delegate(inner);


outer.next();
outer.next();
outer.next();
outer.next();
outer.next();
outer.next();

assertEq(log, "ddddddv");


assertThrowsInstanceOf(outer.next.bind(outer), TypeError);


assertEq(log, "ddddddv");

if (typeof reportCompare == "function")
    reportCompare(true, true);

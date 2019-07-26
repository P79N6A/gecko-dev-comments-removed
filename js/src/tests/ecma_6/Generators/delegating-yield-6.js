


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
        log += 'n';
        return {
            get done() { log += "d"; return count-- == 0; },
            get value() { log += "v"; return val; }
        }
    }

    function iterator() {
        log += 'i';
        return this;
    }

    this.next = next;
    this[std_iterator] = iterator;
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

assertEq(log, "indndndndndndv");


assertThrowsInstanceOf(outer.next.bind(outer), TypeError);


assertEq(log, "indndndndndndv");

if (typeof reportCompare == "function")
    reportCompare(true, true);

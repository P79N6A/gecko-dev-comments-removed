

















TestNumber.prototype = new Number();

function TestNumber(value) {
    this.value = value;
    this.valueOfCalled = false;
}

TestNumber.prototype = {
    valueOf: function() {
        this.valueOfCalled = true;
        return this.value;
    }
}


function test(func ) {
    var args = Array.prototype.slice.call(arguments, 1);
    func.apply(null, args);

    for (var i = 0; i < args.length; ++i)
        assertEq(args[i].valueOfCalled, true);
}





var y = new TestNumber(1);
test(Math.atan2, y);

var x = new TestNumber(1);
var y = new TestNumber(2);
test(Math.atan2, y, x);





























var x = new TestNumber(1);
test(Math.max, x);

var x = new TestNumber(1);
var y = new TestNumber(2);
test(Math.max, x, y);

var x = new TestNumber(1);
var y = new TestNumber(2);
var z = new TestNumber(3);
test(Math.max, x, y, z);


var x = new TestNumber(1);
test(Math.min, x);

var x = new TestNumber(1);
var y = new TestNumber(2);
test(Math.min, x, y);

var x = new TestNumber(1);
var y = new TestNumber(2);
var z = new TestNumber(3);
test(Math.min, x, y, z);


var x = new TestNumber(1);
test(Math.pow, x);

var x = new TestNumber(1);
var y = new TestNumber(2);
test(Math.pow, x, y);

reportCompare(0, 0, "ok");

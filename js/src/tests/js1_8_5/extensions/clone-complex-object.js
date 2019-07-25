



function isClone(a, b) {
    var stack = [[a, b]];
    var memory = new WeakMap();
    var rmemory = new WeakMap();

    while (stack.length > 0) {
        var pair = stack.pop();
        var x = pair[0], y = pair[1];
        if (typeof x !== "object" || x === null) {
            
            if (x !== y)
                return false;
        } else if (x instanceof Date) {
            if (x.getTime() !== y.getTime())
                return false;
        } else if (memory.has(x)) {
            
            if (y !== memory.get(x))
                return false;
            assertEq(rmemory.get(y), x);
        } else {
            
	          
            if (rmemory.has(y))
                return false;

            
            var xcls = Object.prototype.toString.call(x);
            var ycls = Object.prototype.toString.call(y);
            if (xcls !== ycls)
                return false;

            
            assertEq(xcls === "[object Object]" || xcls === "[object Array]",
                     true);

            
            var xk = Object.keys(x), yk = Object.keys(y);
            if (xk.length !== yk.length)
                return false;
            for (var i = 0; i < xk.length; i++) {
                
                if (xk[i] !== yk[i])
                    return false;

                
                stack.push([x[xk[i]], y[yk[i]]]);
            }

            
            memory.set(x, y);
            rmemory.set(y, x);
        }
    }
    return true;
}

function check(a) {
    assertEq(isClone(a, deserialize(serialize(a))), true);
}





var a = [];
a[0] = a;
check(a);


var b = {};
b.next = b;
check(b);


var a = [];
var b = {};
var c = {};
a[0] = b;
a[1] = b;
a[2] = b;
b.next = a;
check(a);
check(b);


check(new Date);


a = [];
b = a;
for (var i = 0; i < 10000; i++) {
    b[0] = {};
    b[1] = [];
    b = b[1];
}
b[0] = {owner: a};
b[1] = [];
check(a);

reportCompare(0, 0, 'ok');

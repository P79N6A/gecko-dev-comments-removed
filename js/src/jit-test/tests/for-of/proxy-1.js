

function iterableProxy(arr) {
    return Proxy.create({
        getPropertyDescriptor: function (name) {
            for (var obj = arr; obj; obj = Object.getPrototypeOf(obj)) {
                var desc = Object.getOwnPropertyDescriptor(obj, name);
                if (desc)
                    return desc;
            }
            return undefined;
        }
    });
}

var s = '';
var arr = ['a', 'b', 'c', 'd'];
var p = iterableProxy(arr);




for (var i = 0; i < 2; i++) {
    var j = 0;
    for (var x of p)
        assertEq(x, arr[j++]);
    assertEq(j, arr.length);
}

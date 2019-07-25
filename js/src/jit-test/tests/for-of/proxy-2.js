

var arr = ['a', 'b', 'c', 'd'];
var proxy = Proxy.create({
    getPropertyDescriptor: function (name) {
        if (name == 'iterator') {
            return {
                configurable: false,
                enumerable: false,
                writeable: false,
                value:  function () {
                    for (var i = 0; i < arr.length; i++)
                        yield arr[i];
                }
            };
        }

        
        for (var obj = arr; obj; obj = Object.getPrototypeOf(obj)) {
            var desc = Object.getOwnPropertyDescriptor(obj, name);
            if (desc)
                return desc;
        }
        return undefined;
    }
});

for (var i = 0; i < 2; i++)
    assertEq([v for (v of proxy)].join(","), "a,b,c,d");












function check(b, desc) {
    function classOf(obj) {
        return Object.prototype.toString.call(obj);
    }

    function ownProperties(obj) {
        return Object.getOwnPropertyNames(obj).
            map(function (p) { return [p, Object.getOwnPropertyDescriptor(obj, p)]; });
    }

    function isCloneable(pair) {
        return typeof pair[0] === 'string' && pair[1].enumerable;
    }

    function notIndex(p) {
        var u = p >>> 0;
        return !("" + u == p && u != 0xffffffff);
    }

    function assertIsCloneOf(a, b, path) {
        assertEq(a === b, false);

        var ca = classOf(a);
        assertEq(ca, classOf(b), path);

        assertEq(Object.getPrototypeOf(a),
                 ca == "[object Object]" ? Object.prototype : Array.prototype,
                 path);

        
        
        
        
        var pb = ownProperties(b).filter(isCloneable);
        var pa = ownProperties(a);
        for (var i = 0; i < pa.length; i++) {
            assertEq(typeof pa[i][0], "string", "clone should not have E4X properties " + path);
            if (!pa[i][1].enumerable) {
                if (Array.isArray(a) && pa[i][0] == "length") {
                    
                    pa.splice(i, 1);
                    i--;
                } else {
                    throw new Error("non-enumerable clone property " + uneval(pa[i][0]) + " " + path);
                }
            }
        }

        
        
        var aNames = pa.map(function (pair) { return pair[1]; }).filter(notIndex);
        var bNames = pa.map(function (pair) { return pair[1]; }).filter(notIndex);
        assertEq(aNames.join(","), bNames.join(","), path);

        
        function byName(a, b) { a = a[0]; b = b[0]; return a < b ? -1 : a === b ? 0 : 1; }
        pa.sort(byName);
        pb.sort(byName);
        assertEq(pa.length, pb.length, "should see the same number of properties " + path);
        for (var i = 0; i < pa.length; i++) {
            var aName = pa[i][0];
            var bName = pb[i][0];
            assertEq(aName, bName, path);

            var path2 = path + "." + aName;
            var da = pa[i][1];
            var db = pb[i][1];
            assertEq(da.configurable, true, path2);
            assertEq(da.writable, true, path2);
            assertEq("value" in da, true, path2);
            var va = da.value;
            var vb = b[pb[i][0]];
            if (typeof va === "object" && va !== null)
                queue.push([va, vb, path2]);
            else
                assertEq(va, vb, path2);
        }
    }

    var banner = "while testing clone of " + (desc || uneval(b));
    var a = deserialize(serialize(b));
    var queue = [[a, b, banner]];
    while (queue.length) {
        var triple = queue.shift();
        assertIsCloneOf(triple[0], triple[1], triple[2]);
    }

    return a; 
}

function test() {
    check({});
    check([]);
    check({x: 0});
    check({x: 0.7, p: "forty-two", y: null, z: undefined});
    check(Array.prototype);
    check(Object.prototype);

    
    var b, a;

    
    b = [, 1, 2, 3];
    b.expando = true;
    b[5] = 5;
    b[0] = 0;
    b[4] = 4;
    delete b[2];
    check(b);

    
    
    
    b = {};
    Object.defineProperties(b, {
        x: {enumerable: true, get: function () { return 12479; }},
        y: {enumerable: true, configurable: true, writable: false, value: 0},
        z: {enumerable: true, configurable: false, writable: true, value: 0},
        hidden: {enumerable:false, value: 1334}});
    check(b);

    
    b = {"-1": -1,
         0xffffffff: null,
         0x100000000: null,
         "": 0,
         "\xff\x7f\u7fff\uffff\ufeff\ufffe": 1, 
         "\ud800 \udbff \udc00 \udfff": 2}; 
    check(b);

    b = [];
    b[-1] = -1;
    b[0xffffffff] = null;
    b[0x100000000] = null;
    b[""] = 0;
    b["\xff\x7f\u7fff\uffff\ufeff\ufffe"] = 1;
    b["\ud800 \udbff \udc00 \udfff"] = 2;
    check(b);

    
    b = Array(5);
    assertEq(b.length, 5);
    a = check(b);
    assertEq(a.length, 0);

    b[1] = "ok";
    a = check(b);
    assertEq(a.length, 2);

    
    b = Object.create({x:1});
    b.y = 2;
    b.z = 3;
    check(b);

    
    var same = {};
    b = {one: same, two: same};
    a = check(b);
    assertEq(a.one === a.two, true);

    b = [same, same];
    a = check(b);
    assertEq(a[0] === a[1], true);

    
    b = {};
    var current = b;
    for (var i = 0; i < 10000; i++) {
        var next = {};
        current['x' + i] = next;
        current = next;
    }
    check(b, "deepObject");  

    















    
    check([0, 1, 2, , 4, 5, 6]);

    
    b = [];
    b[255] = 1;
    check(b);
    assertEq(serialize(b).length < 255, true);

    
    
    b = Object.create({y: 2}, 
                      {x: {enumerable: true,
                           configurable: true,
                           get: function() { if (this.hasOwnProperty("y")) delete this.y; return 1; }},
                       y: {enumerable: true,
                           configurable: true,
                           writable: true,
                           value: 3}});
    check(b, "selfModifyingObject");

    
    var uri = "http://example.net";
    b = {x: 1, y: 2};
    Object.defineProperty(b, QName(uri, "x"), {enumerable: true, value: 3});
    Object.defineProperty(b, QName(uri, "y"), {enumerable: true, value: 5});
    check(b);
}

test();
reportCompare(0, 0, 'ok');

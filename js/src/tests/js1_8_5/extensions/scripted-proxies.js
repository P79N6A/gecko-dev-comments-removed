






var BUGNUMBER = 546590;
var summary = 'basic scripted proxies tests';
var actual = '';
var expect = '';


test();


function test() {
    enterFunc ('test');
    printBugNumber(BUGNUMBER);
    printStatus(summary);

    testObj({ foo: 1, bar: 2 });
    testObj({ 1: 2, 3: 4 });
    testObj([ 1, 2, 3 ]);
    testObj(new Date());
    testObj(new Array());
    testObj(new RegExp());
    testObj(Date);
    testObj(Array);
    testObj(RegExp);

    
    var proxy = Proxy.createFunction({
        get: function(obj, name) {
            return Function.prototype[name];
        },
        getOwnPropertyDescriptor: function(obj, name) {
            return Object.getOwnPropertyDescriptor(Function.prototype, name);
        },
        fix: function() {
            return ({});
        }
    }, function() { return "call"; });

    assertEq(proxy(), "call");
    assertEq(Function.prototype.bind.call(proxy)(), "call");
    assertEq(typeof proxy, "function");
    if ("isTrapping" in Proxy) {
	assertEq(Proxy.isTrapping(proxy), true);
	assertEq(Proxy.fix(proxy), true);
	assertEq(Proxy.isTrapping(proxy), false);
	assertEq(typeof proxy, "function");
	assertEq(proxy(), "call");
    }

    
    var proxy = Proxy.createFunction({
        get: function(obj, name) {
            return Function.prototype[name];
        },
        getOwnPropertyDescriptor: function(obj, name) {
            return Object.getOwnPropertyDescriptor(Function.prototype, name);
        },
        fix: function() {
            return ({});
        }
    },
    function() { var x = {}; x.origin = "call"; return x; },
    function() { var x = {}; x.origin = "new"; return x; })

    assertEq(proxy().origin, "call");
    assertEq(Function.prototype.bind.call(proxy)().origin, "call");
    assertEq((new proxy()).origin, "new");
    assertEq(new (Function.prototype.bind.call(proxy))().origin, "new");
    if ("fix" in Proxy) {
	assertEq(Proxy.fix(proxy), true);
	assertEq(proxy().origin, "call");
	assertEq((new proxy()).origin, "new");
    }

    
    var proxy = Proxy.createFunction({
        get: function(obj, name) { return Function.prototype[name]; },
        fix: function() { return ({}); }
    },
    function() { this.origin = "new"; return "new-ret"; });

    assertEq((new proxy()).origin, "new");
    if ("fix" in Proxy) {
        assertEq(Proxy.fix(proxy), true);
        assertEq((new proxy()).origin, "new");
    }

    
    var proxy = Proxy.create({ get: function(obj,name) { return function(a,b,c) { return name + uneval([a,b,c]); } }});
    assertEq(proxy.foo(1,2,3), "foo[1, 2, 3]");

    reportCompare(0, 0, "done.");

    exitFunc ('test');
}


function noopHandlerMaker(obj) {
    return {
	getOwnPropertyDescriptor: function(name) {
	    var desc = Object.getOwnPropertyDescriptor(obj);
	    
	    desc.configurable = true;
	    return desc;
	},
	getPropertyDescriptor: function(name) {
	    var desc = Object.getPropertyDescriptor(obj); 
	    
	    desc.configurable = true;
	    return desc;
	},
	getOwnPropertyNames: function() {
	    return Object.getOwnPropertyNames(obj);
	},
	defineProperty: function(name, desc) {
	    return Object.defineProperty(obj, name, desc);
	},
	delete: function(name) { return delete obj[name]; },
	fix: function() {
	    
	    
	    
	    
	    var props = {};
	    for (x in obj)
		props[x] = Object.getOwnPropertyDescriptor(obj, x);
	    return props;
	},
 	has: function(name) { return name in obj; },
	hasOwn: function(name) { return ({}).hasOwnProperty.call(obj, name); },
	get: function(receiver, name) { return obj[name]; },
	set: function(receiver, name, val) { obj[name] = val; return true; }, 
	enumerate: function() {
	    var result = [];
	    for (name in obj) { result.push(name); };
	    return result;
	},
	keys: function() { return Object.keys(obj); }
    };
};

function testNoopHandler(obj, proxy) {
    
    for (x in obj)
	assertEq(obj[x], proxy[x]);
    for (x in proxy)
	assertEq(obj[x], proxy[x]);
    
    var a = [], b = [];
    for (x in obj)
	a.push(x);
    for (x in proxy)
	b.push(x);
    assertEq(uneval(a), uneval(b));
}

function testObj(obj) {
    var proxy = Proxy.create(noopHandlerMaker(obj));
    testNoopHandler(obj, proxy);
    assertEq(typeof proxy, "object");
    if ("isTrapping" in Proxy) {
	assertEq(Proxy.isTrapping(proxy), true);
	assertEq(Proxy.fix(proxy), true);
	assertEq(Proxy.isTrapping(proxy), false);
	assertEq(typeof proxy, "object");
	testNoopHandler(obj, proxy);
    }
}

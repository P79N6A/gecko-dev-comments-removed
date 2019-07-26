
var BUGNUMBER = 578700;
var summary = 'BinaryData ArrayType implementation';

function assertThrows(f) {
    var ok = false;
    try {
        f();
    } catch (exc) {
        ok = true;
    }
    if (!ok)
        throw new TypeError("Assertion failed: " + f + " did not throw as expected");
}

function runTests() {
    print(BUGNUMBER + ": " + summary);

    assertEq(typeof ArrayType.prototype.prototype.forEach == "function", true);
    assertEq(typeof ArrayType.prototype.prototype.subarray == "function", true);

    assertThrows(function() ArrayType(uint8, 10));
    assertThrows(function() new ArrayType());
    assertThrows(function() new ArrayType(""));
    assertThrows(function() new ArrayType(5));
    assertThrows(function() new ArrayType(uint8, -1));
    var A = new ArrayType(uint8, 10);
    assertEq(A.__proto__, ArrayType.prototype);
    assertEq(A.length, 10);
    assertEq(A.elementType, uint8);
    assertEq(A.bytes, 10);
    assertEq(A.toString(), "ArrayType(uint8, 10)");

    assertEq(A.prototype.__proto__, ArrayType.prototype.prototype);
    assertEq(typeof A.prototype.fill, "function");

    var a = new A();
    assertEq(a.__proto__, A.prototype);
    assertEq(a.length, 10);

    assertThrows(function() a.length = 2);

    for (var i = 0; i < a.length; i++)
        a[i] = i*2;

    for (var i = 0; i < a.length; i++)
        assertEq(a[i], i*2);


    
    assertThrows(function() a[i] = 5);

    assertEq(a[a.length], undefined);

    
    var b = new A(a);
    for (var i = 0; i < a.length; i++)
        assertEq(a[i], i*2);

    var b = new A([0, 1, 0, 1, 0, 1, 0, 1, 0, 1]);
    for (var i = 0; i < b.length; i++)
        assertEq(b[i], i%2);


    assertThrows(function() new A(5));
    assertThrows(function() new A(/fail/));
    
    assertThrows(function() new A([0, 1, 0, 1, 0, 1, 0, 1, 0]));

    var Vec3 = new ArrayType(float32, 3);
    var Sprite = new ArrayType(Vec3, 3); 
    assertEq(Sprite.elementType, Vec3);
    assertEq(Sprite.elementType.elementType, float32);


    var mario = new Sprite();
    
    mario[0] = new Vec3([1, 0, 0]);
    
    mario[1] = [1, 1.414, 3.14];

    assertEq(mario[0].length, 3);
    assertEq(mario[0][0], 1);
    assertEq(mario[0][1], 0);
    assertEq(mario[0][2], 0);

    assertThrows(function() mario[1] = 5);
    mario[1][1] = [];
    assertEq(Number.isNaN(mario[1][1]), true);


    
    var AllSprites = new ArrayType(Sprite, 65536);
    var as = new AllSprites();
    assertEq(as.length, 65536);


    as.foo = "bar";

    var indexPropDesc = Object.getOwnPropertyDescriptor(as, '0');
    assertEq(typeof indexPropDesc == "undefined", false);
    assertEq(indexPropDesc.configurable, false);
    assertEq(indexPropDesc.enumerable, true);
    assertEq(indexPropDesc.writable, true);


    var lengthPropDesc = Object.getOwnPropertyDescriptor(as, 'length');
    assertEq(typeof lengthPropDesc == "undefined", false);
    assertEq(lengthPropDesc.configurable, false);
    assertEq(lengthPropDesc.enumerable, false);
    assertEq(lengthPropDesc.writable, false);

    assertThrows(function() Object.defineProperty(o, "foo", { value: "bar" }));

    
    var AA = new ArrayType(new ArrayType(uint8, 5), 5);
    var aa = new AA();
    var aa0 = aa[0];
    aa[0] = [0,1,2,3,4];
    for (var i = 0; i < aa0.length; i++)
        assertEq(aa0[i], i);

    if (typeof reportCompare === "function")
        reportCompare(true, true);
    print("Tests complete");
}

runTests();

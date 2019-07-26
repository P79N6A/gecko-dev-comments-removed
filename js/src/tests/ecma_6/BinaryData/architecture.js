
var BUGNUMBER = 578700;
var summary = 'Binary Data class diagram';

function assertNotEq(a, b) {
    var ok = false;
    try {
        assertEq(a, b);
    } catch(exc) {
        ok = true;
    }

    if (!ok)
        throw new TypeError("Assertion failed: assertNotEq(" + a + " " + b + ")");
}

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

    assertEq(Data.__proto__, Function.prototype);
    assertEq(Data.prototype.__proto__, Object.prototype);
    assertEq(Data.prototype.constructor, Data);
    assertEq(typeof Data.prototype.update === "function", true);

    assertEq(Type.__proto__, Function.prototype);
    assertEq(Type.prototype, Data);

    assertEq(ArrayType.__proto__, Type);
    assertEq(ArrayType.prototype.__proto__, Type.prototype);
    assertEq(typeof ArrayType.prototype.repeat === "function", true);

    assertEq(ArrayType.prototype.prototype.__proto__, Data.prototype);

    assertEq(StructType.__proto__, Type);
    assertEq(StructType.prototype.__proto__, Type.prototype);
    assertEq(StructType.prototype.prototype.__proto__, Data.prototype);

    
    Type = function() {
        return 42;
    }

    Data = function() {
        return 43;
    }

    assertNotEq(ArrayType.prototype.__proto__, Type.prototype);
    assertNotEq(ArrayType.prototype.prototype.__proto__, Data.prototype);

    assertNotEq(StructType.prototype.__proto__, Type.prototype);
    assertNotEq(StructType.prototype.prototype.__proto__, Data.prototype);

    if (typeof reportCompare === "function")
        reportCompare(true, true);
    print("Tests complete");
}

runTests();

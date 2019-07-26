

var unsealed = {};
var sealed = Object.seal({});
var handler = {};

assertEq(Object.isExtensible(unsealed), true);
assertEq(Object.isExtensible(sealed), false);

var targetSealed = new Proxy(sealed, handler);
var targetUnsealed = new Proxy(unsealed, handler);

var handlerCalled = false;



assertEq(Object.isExtensible(targetSealed), false, "Must forward to target without hook.");
assertEq(Object.isExtensible(targetUnsealed), true, "Must forward to target without hook.");


function ensureCalled() { handlerCalled = true; return true; }
var proxyTarget = new Proxy({}, { isExtensible : ensureCalled });
assertEq(Object.isExtensible(new Proxy(proxyTarget, {})), true, "Must forward to target without hook.");
assertEq(handlerCalled, true, "Must forward to target without hook.");



function testExtensible(obj, shouldThrow, expectedResult)
{
    handlerCalled = false;
    if (shouldThrow)
        assertThrowsInstanceOf(function () { Object.isExtensible(obj); },
                               TypeError, "Must throw if handler and target disagree.");
    else
        assertEq(Object.isExtensible(obj), expectedResult, "Must return the correct value.");
    assertEq(handlerCalled, true, "Must call handler trap if present");
}


function fakeSealed() { handlerCalled = true; return false; }
handler.isExtensible = fakeSealed;
testExtensible(targetSealed, false, false);
testExtensible(targetUnsealed, true);


function fakeUnsealed() { handlerCalled = true; return true; }
handler.isExtensible = fakeUnsealed;
testExtensible(targetSealed, true);
testExtensible(targetUnsealed, false, true);



function makeSealedTruth(target) { handlerCalled = true; Object.preventExtensions(target); return false; }
function makeSealedLie(target) { handlerCalled = true; Object.preventExtensions(target); return true; }
handler.isExtensible = makeSealedTruth;
testExtensible(new Proxy({}, handler), false, false);
handler.isExtensible = makeSealedLie;
testExtensible(new Proxy({}, handler), true);


function falseyNonBool() { handlerCalled = true; return undefined; }
handler.isExtensible = falseyNonBool;
testExtensible(targetSealed, false, false);
testExtensible(targetUnsealed, true);

function truthyNonBool() { handlerCalled = true; return {}; }
handler.isExtensible = truthyNonBool;
testExtensible(targetSealed, true);
testExtensible(targetUnsealed, false, true);


function ExtensibleError() { }
ExtensibleError.prototype = new Error();
ExtensibleError.prototype.constructor = ExtensibleError;
function throwFromTrap() { throw new ExtensibleError(); }
handler.isExtensible = throwFromTrap;



assertThrowsInstanceOf(function () { Object.isExtensible(targetSealed); },
                       ExtensibleError, "Must throw if the trap does.");
assertThrowsInstanceOf(function () { Object.isFrozen(targetSealed); },
                       ExtensibleError, "Must throw if the trap does.");
assertThrowsInstanceOf(function () { Object.isSealed(targetSealed); },
                       ExtensibleError, "Must throw if the trap does.");



function recurse() { return Object.isExtensible(targetSealed); }
handler.isExtensible = recurse;
assertThrowsInstanceOf(function () { Object.isExtensible(targetSealed); },
                       InternalError, "Should allow and detect infinite recurison.");

reportCompare(0, 0, "OK");

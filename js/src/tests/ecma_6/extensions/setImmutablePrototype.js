




var gTestfile = "setImmutablePrototype.js";

var BUGNUMBER = 1052139;
var summary =
  "Implement JSAPI and a shell function to prevent modifying an extensible " +
  "object's [[Prototype]]";

print(BUGNUMBER + ": " + summary);





if (typeof evaluate !== "function")
{
  
  
  evaluate = eval;
}

var usingRealSetImmutablePrototype = true;

if (typeof setImmutablePrototype !== "function")
{
  usingRealSetImmutablePrototype = false;

  if (typeof SpecialPowers !== "undefined")
  {
    setImmutablePrototype =
      SpecialPowers.Cu.getJSTestingFunctions().setImmutablePrototype;
  }
}

if (typeof wrap !== "function")
{
  
  wrap = function(x) { return x; };
}

function setViaProtoSetter(obj, newProto)
{
  var setter =
    Object.getOwnPropertyDescriptor(Object.prototype, "__proto__").set;
  setter.call(obj, newProto);
}

function checkPrototypeMutationFailure(obj, desc)
{
  var initialProto = Object.getPrototypeOf(obj);

  
  var newProto = Object.create(null);

  function tryMutate(func, method)
  {
    try
    {
      func(obj, newProto);
      throw new Error(desc + ": no error thrown, prototype " +
                      (Object.getPrototypeOf(obj) === initialProto
                       ? "wasn't"
                       : "was") +
                      " changed");
    }
    catch (e)
    {
      
      
      
      assertEq(e instanceof TypeError, true,
               desc + ": should have thrown TypeError setting [[Prototype]] " +
               "via " + method + ", got " + e);
      assertEq(Object.getPrototypeOf(obj), initialProto,
               desc + ": shouldn't observe [[Prototype]] change");
    }
  }

  tryMutate(Object.setPrototypeOf, "Object.setPrototypeOf");
  tryMutate(setViaProtoSetter, "__proto__ setter");
}

function runNormalTests(global)
{
  if (typeof setImmutablePrototype !== "function")
  {
    print("no usable setImmutablePrototype function available, skipping tests");
    return;
  }

  

  var emptyLiteral = global.evaluate("({})");
  assertEq(setImmutablePrototype(emptyLiteral), true);
  checkPrototypeMutationFailure(emptyLiteral, "empty literal");

  

  var nullProto = global.Object.create(null);
  assertEq(setImmutablePrototype(nullProto), true);
  checkPrototypeMutationFailure(nullProto, "nullProto");

  
  if (!usingRealSetImmutablePrototype)
    return;

  

  var emptyTarget = global.evaluate("({})");
  var directProxy = new global.Proxy(emptyTarget, {});
  assertEq(setImmutablePrototype(directProxy), true);
  checkPrototypeMutationFailure(directProxy, "direct proxy to empty target");
  checkPrototypeMutationFailure(emptyTarget, "empty target");

  var anotherTarget = global.evaluate("({})");
  var anotherDirectProxy = new global.Proxy(anotherTarget, {});
  assertEq(setImmutablePrototype(anotherTarget), true);
  checkPrototypeMutationFailure(anotherDirectProxy, "another direct proxy to empty target");
  checkPrototypeMutationFailure(anotherTarget, "another empty target");

  var nestedTarget = global.evaluate("({})");
  var nestedProxy = new global.Proxy(new Proxy(nestedTarget, {}), {});
  assertEq(setImmutablePrototype(nestedProxy), true);
  checkPrototypeMutationFailure(nestedProxy, "nested proxy to empty target");
  checkPrototypeMutationFailure(nestedTarget, "nested target");

  

  var revocableTarget = global.evaluate("({})");
  var revocable = global.Proxy.revocable(revocableTarget, {});
  assertEq(setImmutablePrototype(revocable.proxy), true);
  checkPrototypeMutationFailure(revocableTarget, "revocable target");
  checkPrototypeMutationFailure(revocable.proxy, "revocable proxy");

  assertEq(revocable.revoke(), undefined);
  try
  {
    setImmutablePrototype(revocable.proxy);
    throw new Error("expected to throw on revoked proxy");
  }
  catch (e)
  {
    
    
    
    
    assertEq(e instanceof global.TypeError, true,
             "expected TypeError, instead threw " + e);
  }

  var anotherRevocableTarget = global.evaluate("({})");
  assertEq(setImmutablePrototype(anotherRevocableTarget), true);
  checkPrototypeMutationFailure(anotherRevocableTarget, "another revocable target");

  var anotherRevocable = global.Proxy.revocable(anotherRevocableTarget, {});
  checkPrototypeMutationFailure(anotherRevocable.proxy, "another revocable target");

  assertEq(anotherRevocable.revoke(), undefined);
  try
  {
    var rv = setImmutablePrototype(anotherRevocable.proxy);
    throw new Error("expected to throw on another revoked proxy, returned " + rv);
  }
  catch (e)
  {
    
    assertEq(e instanceof global.TypeError, true,
             "expected TypeError, instead threw " + e);
  }

  
  var oldProto = {};
  var indirectProxy = global.Proxy.create({}, oldProto);
  assertEq(setImmutablePrototype(indirectProxy), true);
  assertEq(Object.getPrototypeOf(indirectProxy), oldProto);
  checkPrototypeMutationFailure(indirectProxy, "indirectProxy");

  var indirectFunctionProxy = global.Proxy.createFunction({}, function call() {});
  assertEq(setImmutablePrototype(indirectFunctionProxy), true);
  assertEq(Object.getPrototypeOf(indirectFunctionProxy), global.Function.prototype);
  checkPrototypeMutationFailure(indirectFunctionProxy, "indirectFunctionProxy");
}

var global = this;
runNormalTests(global); 

if (typeof newGlobal === "function")
{
  var otherGlobal = newGlobal();
  var subsumingNothing = newGlobal({ principal: 0 });
  var subsumingEverything = newGlobal({ principal: ~0 });

  runNormalTests(otherGlobal); 
  runNormalTests(subsumingNothing);
  runNormalTests(subsumingEverything);
}



if (typeof reportCompare === "function")
  reportCompare(true, true);

print("Tests complete");

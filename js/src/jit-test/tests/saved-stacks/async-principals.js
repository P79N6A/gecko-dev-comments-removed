


function checkVisibleStack(stackFrame, expectedFrames) {
  
  
  var stringFrames = stackFrame.toString().split("\n");

  while (expectedFrames.length) {
    let expectedFrame = expectedFrames.shift();
    let stringFrame = stringFrames.shift();

    
    assertEq(stackFrame.functionDisplayName, expectedFrame.name);
    assertEq(stackFrame.asyncCause, expectedFrame.asyncCause);

    
    let expectedStart =
      (expectedFrame.asyncCause ? expectedFrame.asyncCause + "*" : "") +
      expectedFrame.name;
    assertEq(stringFrame.replace(/@.*$/, ""), expectedStart);

    
    if (expectedFrames.length && expectedFrames[0].asyncCause) {
      assertEq(stackFrame.parent, null);
      stackFrame = stackFrame.asyncParent;
    } else {
      assertEq(stackFrame.asyncParent, null);
      stackFrame = stackFrame.parent;
    }
  }
}

var low = newGlobal({ principal: 0 });
var high = newGlobal({ principal: 0xfffff });
























function a() {
  b();
}
function b() {
  callFunctionWithAsyncStack(c, saveStack(), "BtoC");
}
function c() {
  callFunctionWithAsyncStack(d, saveStack(), "CtoD");
}
function d() {
  let stackD = saveStack();

  print("high.checkVisibleStack(stackD)");
  checkVisibleStack(stackD, [
    { name: "d", asyncCause: null   },
    { name: "c", asyncCause: "CtoD" },
    { name: "b", asyncCause: "BtoC" },
    { name: "a", asyncCause: null   },
  ]);

  let stackE = e(saveStack(0, low));

  print("high.checkVisibleStack(stackE)");
  checkVisibleStack(stackE, [
    { name: "e", asyncCause: null   },
    { name: "d", asyncCause: null   },
    { name: "c", asyncCause: "CtoD" },
    { name: "b", asyncCause: "BtoC" },
    { name: "a", asyncCause: null   },
  ]);
}
function e(stackD) {
  print("low.checkVisibleStack(stackD)");
  checkVisibleStack(stackD, [
    { name: "a", asyncCause: "Async" },
  ]);

  let stackE = saveStack();

  print("low.checkVisibleStack(stackE)");
  checkVisibleStack(stackE, [
    { name: "e", asyncCause: null    },
    { name: "a", asyncCause: "Async" },
  ]);

  return saveStack(0, high);
}

























function u() {
  callFunctionWithAsyncStack(v, saveStack(), "UtoV");
  w();
}
function v() {
  let stackV = saveStack();
  print("high.checkVisibleStack(stackV)");
  checkVisibleStack(stackV, [
    { name: "v", asyncCause: null   },
    { name: "u", asyncCause: "UtoV" },
  ]);

  let xToCall = x.bind(undefined, saveStack(0, low));

  let stackX = callFunctionWithAsyncStack(xToCall, saveStack(), "VtoX");

  print("high.checkVisibleStack(stackX)");
  checkVisibleStack(stackX, [
    { name: "x", asyncCause: null   },
    { name: "v", asyncCause: "VtoX" },
    { name: "u", asyncCause: "UtoV" },
  ]);

  let stackY = y();

  print("high.checkVisibleStack(stackY)");
  checkVisibleStack(stackY, [
    { name: "y", asyncCause: null   },
    { name: "v", asyncCause: null   },
    { name: "u", asyncCause: "UtoV" },
  ]);
}
function w() {
  let stackZ = callFunctionWithAsyncStack(z, saveStack(), "WtoZ");

  print("high.checkVisibleStack(stackZ)");
  checkVisibleStack(stackZ, [
    { name: "z", asyncCause: null   },
    { name: "w", asyncCause: "WtoZ" },
    { name: "u", asyncCause: null   },
  ]);
}
function x(stackV) {
  print("low.checkVisibleStack(stackV)");
  checkVisibleStack(stackV, [
    { name: "u", asyncCause: "UtoV" },
  ]);

  let stackX = saveStack();

  print("low.checkVisibleStack(stackX)");
  checkVisibleStack(stackX, [
    { name: "x", asyncCause: null   },
    { name: "u", asyncCause: "UtoV" },
  ]);

  return saveStack(0, high);
}
function y() {
  let stackY = saveStack();

  print("low.checkVisibleStack(stackY)");
  checkVisibleStack(stackY, [
    { name: "y", asyncCause: null   },
    { name: "u", asyncCause: "UtoV" },
  ]);

  return saveStack(0, high);
}
function z() {
  let stackZ = saveStack();

  print("low.checkVisibleStack(stackZ)");
  checkVisibleStack(stackZ, [
    { name: "z", asyncCause: null    },
    { name: "u", asyncCause: "Async" },
  ]);

  return saveStack(0, high);
}


low .eval(a.toSource());
high.eval(b.toSource());
high.eval(c.toSource());
high.eval(d.toSource());
low .eval(e.toSource());

low .b = high.b;
high.e = low .e;

low .eval(u.toSource());
high.eval(v.toSource());
high.eval(w.toSource());
low .eval(x.toSource());
low .eval(y.toSource());
low .eval(z.toSource());

low .v = high.v;
low .w = high.w;
high.x = low .x;
high.y = low .y;
high.z = low .z;

low .high = high;
high.low  = low;

low .eval(checkVisibleStack.toSource());
high.eval(checkVisibleStack.toSource());


low.a();
low.u();

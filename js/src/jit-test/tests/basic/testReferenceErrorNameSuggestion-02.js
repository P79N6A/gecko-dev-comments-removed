

const name = "a".repeat(10000);
this[name] = () => {};

let e;
try {
  eval(name + "a()");
} catch (ee) {
  e = ee;
}

assertEq(e !== undefined, true);
assertEq(e.name, "ReferenceError");

assertEq(e.message.contains("did you mean"), false);

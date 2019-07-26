


const filter = (function iife() {
  try {
    [3].filter(n => { throw saveStack() });
  } catch (s) {
    return s;
  }
}());

assertEq(filter.parent.functionDisplayName, "iife");

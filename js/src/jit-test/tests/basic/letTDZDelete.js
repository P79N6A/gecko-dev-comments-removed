function assertThrowsReferenceError(f) {
  var e = null;
  try {
    f();
  } catch (ex) {
    e = ex;
  }
  assertEq(e instanceof ReferenceError, true);
}

assertThrowsReferenceError(function () { delete x; let x; });


function constIsLexical() {
  try {
    (function () { z++; const z; })();
    return false;
  } catch (e) {
    return true;
  }
}
if (constIsLexical())
  assertThrowsReferenceError(function () { delete x; const x; });

function assertThrowsReferenceError(f) {
  var e = null;
  try {
    f();
  } catch (ex) {
    e = ex;
  }
  assertEq(e instanceof ReferenceError, true);
}


assertThrowsReferenceError(function () { x; let x; });


function constIsLexical() {
  try {
    (function () { z++; const z; })();
    return false;
  } catch (e) {
    return true;
  }
}
if (constIsLexical())
  assertThrowsReferenceError(function () { x; const x; });

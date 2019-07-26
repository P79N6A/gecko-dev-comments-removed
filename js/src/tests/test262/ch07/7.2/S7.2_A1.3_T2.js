










varx=1;
if (x !== 1) {
  $ERROR('#1: varx=1; x === 1. Actual: ' + (x));
}


eval("var\fx=\f1");
if (x !== 1) {
  $ERROR('#2: var\\fx=\\f1; x === 1. Actual: ' + (x));
}



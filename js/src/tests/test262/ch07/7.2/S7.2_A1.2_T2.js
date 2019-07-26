










varx=1;
if (x !== 1) {
  $ERROR('#1: varx=1; x === 1. Actual: ' + (x));
}


eval("var\vx=\v1");
if (x !== 1) {
  $ERROR('#2: var\\vx=\\v1; x === 1. Actual: ' + (x));
}



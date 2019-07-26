










var x = -1;
var x1 = (x %= 2);
if (x1 !== -1) {
  $ERROR('#1: var x = -1; var x1 = (x %= 2); x1 === -1. Actual: ' + (x1));
}


y = -1;
y1 = (y %= 2);
if (y1 !== -1) {
  $ERROR('#2: y = -1; y1 = (y %= 2); y1 === -1. Actual: ' + (y1));
}


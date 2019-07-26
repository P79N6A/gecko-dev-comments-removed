










var x = -1;
var x1 = (x -= 1);
if (x1 !== -2) {
  $ERROR('#1: var x = -1; var x1 = (x -= 1); x1 === -2. Actual: ' + (x1));
}


y = -1;
y1 = (y -= 1);
if (y1 !== -2) {
  $ERROR('#2: y = -1; y1 = (y -= 1); y1 === -2. Actual: ' + (y1));
}


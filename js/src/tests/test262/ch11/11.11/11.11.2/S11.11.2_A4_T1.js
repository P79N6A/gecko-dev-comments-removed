










if (((true || true)) !== true) {
  $ERROR('#1: (true || true) === true');
}


if ((true || false) !== true) {
  $ERROR('#2: (true || false) === true');
}


var x = new Boolean(true);
if ((x || new Boolean(true)) !== x) {
  $ERROR('#3: (var x = new Boolean(true); (x || new Boolean(true)) === x');
}


var x = new Boolean(true);
if ((x || new Boolean(false)) !== x) {
  $ERROR('#4: (var x = new Boolean(true); (x || new Boolean(false)) === x');
}


var x = new Boolean(false);
if ((x || new Boolean(true)) !== x) {
  $ERROR('#5: (var x = new Boolean(false); (x || new Boolean(true)) === x');
}


var x = new Boolean(false);
if ((x || new Boolean(false)) !== x) {
  $ERROR('#6: (var x = new Boolean(false); (x || new Boolean(false)) === x');
}


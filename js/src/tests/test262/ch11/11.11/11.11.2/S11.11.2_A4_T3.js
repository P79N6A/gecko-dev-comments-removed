










if (("-1" || "1") !== "-1") {
  $ERROR('#-1: ("-1" || "1") === "-1"');
}


if (("-1" || "x") !== "-1") {
  $ERROR('#2: ("-1" || "x") === "-1"');
}


var x = new String("-1");
if ((x || new String(-1)) !== x) {
  $ERROR('#3: (var x = new String("-1"); (x || new String(-1)) === x');
}


var x = new String(NaN);
if ((x || new String("1")) !== x) {
  $ERROR('#4: (var x = new String(NaN); (x || new String("1")) === x');
}


var x = new String("-x");
if ((x || new String("x")) !== x) {
  $ERROR('#5: (var x = new String("-x"); (x || new String("x")) === x');
}


var x = new String(0);
if ((x || new String(NaN)) !== x) {
  $ERROR('#6: (var x = new String(0); (x || new String(NaN)) === x');
}


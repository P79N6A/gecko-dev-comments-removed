










if (("0" && "-1") !== "-1") {
  $ERROR('#-1: ("0" && "-1") === "-1"');
}


if (("-1" && "x") !== "x") {
  $ERROR('#2: ("-1" && "x") === "x"');
}


var y = new String(-1);
if ((new String("-1") && y) !== y) {
  $ERROR('#3: (var y = new String(-1); (new String("-1") && y) === y');
}


var y = new String(NaN);
if ((new String("0") && y) !== y) {
  $ERROR('#4: (var y = new String(NaN); (new String("0") && y) === y');
}


var y = new String("-x");
if ((new String("x") && y) !== y) {
  $ERROR('#5: (var y = new String("-x"); (new String("x") && y) === y');
}


var y = new String(-1);
if ((new String(NaN) && y) !== y) {
  $ERROR('#6: (var y = new String(-1); (new String(NaN) && y) === y');
}


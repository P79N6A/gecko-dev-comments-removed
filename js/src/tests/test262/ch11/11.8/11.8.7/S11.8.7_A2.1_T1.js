










if ("MAX_VALUE" in Number !== true) {
  $ERROR('#1: "MAX_VALUE" in Number === true');
}


var x = "MAX_VALUE";
if (x in Number !== true) {
  $ERROR('#2: var x = "MAX_VALUE"; x in Number === true');
}


var y = Number;
if ("MAX_VALUE" in  y !== true) {
  $ERROR('#3: var y = Number; "MAX_VALUE" in y === true');
}


var x = "MAX_VALUE";
var y = Number;
if (x in y !== true) {
  $ERROR('#4: var x = "MAX_VALUE"; var y = Number; x in y === true');
}



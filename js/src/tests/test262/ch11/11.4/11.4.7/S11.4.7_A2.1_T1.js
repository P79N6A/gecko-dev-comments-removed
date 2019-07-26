










if (-1 !== -1) {
  $ERROR('#1: -1 === -1. Actual: ' + (-1));
}


if (-(-1) !== 1) {
  $ERROR('#2: -(-1) === -1. Actual: ' + (-(-1)));
}


var x = -1;
if (-x !== 1) {
  $ERROR('#3: var x = -1; -x === 1. Actual: ' + (-x));
}


var x = -1;
if (-(-x) !== -1) {
  $ERROR('#4: var x = -1; -(-x) === -1. Actual: ' + (-(-x)));
}


var object = new Object();
object.prop = 1;
if (-object.prop !== -1) {
  $ERROR('#5: var object = new Object(); object.prop = -1; -object.prop === -1. Actual: ' + (-object.prop));
}


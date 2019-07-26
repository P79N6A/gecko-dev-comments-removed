










if (~0 !== -1) {
  $ERROR('#1: ~0 === -1. Actual: ' + (~0));
}


if (~(~0) !== 0) {
  $ERROR('#2: ~(~0) === 0. Actual: ' + (~(~0)));
}


var x = 0;
if (~x !== -1) {
  $ERROR('#3: var x = 0; ~x === -1. Actual: ' + (~x));
}


var x = 0;
if (~(~x) !== 0) {
  $ERROR('#4: var x = 0; ~(~x) === 0. Actual: ' + (~(~x)));
}


var object = new Object();
object.prop = 0;
if (~object.prop !== -1) {
  $ERROR('#5: var object = new Object(); object.prop = 0; ~object.prop === -1. Actual: ' + (~object.prop));
}


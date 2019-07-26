










if (!true !== false) {
  $ERROR('#1: !true === false');
}


if (!(!true) !== true) {
  $ERROR('#2: !(!true) === true');
}


var x = true;
if (!x !== false) {
  $ERROR('#3: var x = true; !x === false');
}


var x = true;
if (!(!x) !== true) {
  $ERROR('#4: var x = true; !(!x) === true');
}


var object = new Object();
object.prop = true;
if (!object.prop !== false) {
  $ERROR('#5: var object = new Object(); object.prop = true; !object.prop === false');
}


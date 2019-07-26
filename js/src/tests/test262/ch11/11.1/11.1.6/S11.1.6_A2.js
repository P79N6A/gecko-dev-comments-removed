










if (delete (x) !== true) {
  $ERROR('#1: delete (x) === true');
}


if (typeof (x) !== "undefined") {
  $ERROR('#2: typeof (x) === "undefined". Actual: ' + (typeof (x)));
}

var object = {};

if (delete (object.prop) !== true) {
  $ERROR('#3: var object = {}; delete (object.prop) === true');
}


if (typeof (object.prop) !== "undefined") {
  $ERROR('#4: var object = {}; typeof (object.prop) === "undefined". Actual: ' + (typeof (object.prop)));
}


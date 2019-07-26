









var object = {};


if (typeof object !== "object") {
  $ERROR('#1: var object = {}; typeof object === "object". Actual: ' + (typeof object));
}


if (object instanceof Object !== true) {
  $ERROR('#2: var object = {}; object instanceof Object === true');
}


if (object.toString !== Object.prototype.toString) {
  $ERROR('#3: var object = {}; object.toString === Object.prototype.toString. Actual: ' + (object.toString));
}


if (object.toString() !== "[object Object]") {
  $ERROR('#4: var object = {}; object.toString === "[object Object]". Actual: ' + (object.toString));
}


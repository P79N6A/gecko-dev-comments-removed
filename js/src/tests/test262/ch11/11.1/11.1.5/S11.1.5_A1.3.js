









var object = {"x" : true};


if (typeof object !== "object") {
  $ERROR('#1: var object = {"x" : true}; typeof object === "object". Actual: ' + (typeof object));
}


if (object instanceof Object !== true) {
  $ERROR('#2: var object = {"x" : true}; object instanceof Object === true');
}


if (object.toString !== Object.prototype.toString) {
  $ERROR('#3: var object = {"x" : true}; object.toString === Object.prototype.toString. Actual: ' + (object.toString));
}


if (object["x"] !== true) {
  $ERROR('#4: var object = {"x" : true}; object["x"] === true');
}


if (object.x !== true) {
  $ERROR('#5: var object = {"x" : true}; object.x === true');
}


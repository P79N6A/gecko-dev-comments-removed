









var object = {prop : true};


if (typeof object !== "object") {
  $ERROR('#1: var object = {prop : true}; typeof object === "object". Actual: ' + (typeof object));
}


if (object instanceof Object !== true) {
  $ERROR('#2: var object = {prop : true}; object instanceof Object === true');
}


if (object.toString !== Object.prototype.toString) {
  $ERROR('#3: var object = {prop : true}; object.toString === Object.prototype.toString. Actual: ' + (object.toString));
}


if (object["prop"] !== true) {
  $ERROR('#4: var object = {prop : true}; object["prop"] === true');
}


if (object.prop !== true) {
  $ERROR('#5: var object = {prop : true}; object.prop === true');
}


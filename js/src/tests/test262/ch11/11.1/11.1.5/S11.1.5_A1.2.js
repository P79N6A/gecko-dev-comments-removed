









var object = {1 : true};


if (typeof object !== "object") {
  $ERROR('#1: var object = {1 : true}; typeof object === "object". Actual: ' + (typeof object));
}


if (object instanceof Object !== true) {
  $ERROR('#2: var object = {1 : true}; object instanceof Object === true');
}


if (object.toString !== Object.prototype.toString) {
  $ERROR('#3: var object = {1 : true}; object.toString === Object.prototype.toString. Actual: ' + (object.toString));
}


if (object[1] !== true) {
  $ERROR('#4: var object = {1 : true}; object[1] === true');
}


if (object["1"] !== true) {
  $ERROR('#5: var object = {1 : true}; object["1"] === true');
}



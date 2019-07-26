









var array = [,,,,,];


if (typeof array !== "object") {
  $ERROR('#1: var array = [,,,,,]; typeof array === "object". Actual: ' + (typeof array));
}


if (array instanceof Array !== true) {
  $ERROR('#2: var array = [,,,,,]; array instanceof Array === true');
}


if (array.toString !== Array.prototype.toString) {
  $ERROR('#3: var array = [,,,,,]; array.toString === Array.prototype.toString. Actual: ' + (array.toString));
}


if (array.length !== 5) {
  $ERROR('#4: var array = [,,,,,]; array.length === 5. Actual: ' + (array.length));
}


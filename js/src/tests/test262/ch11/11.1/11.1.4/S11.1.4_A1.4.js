









var array = [,,,1,2];


if (typeof array !== "object") {
  $ERROR('#1: var array = [,,,1,2]; typeof array === "object". Actual: ' + (typeof array));
}


if (array instanceof Array !== true) {
  $ERROR('#2: var array = [,,,1,2]; array instanceof Array === true');
}


if (array.toString !== Array.prototype.toString) {
  $ERROR('#3: var array = [,,,1,2]; array.toString === Array.prototype.toString. Actual: ' + (array.toString));
}


if (array.length !== 5) {
  $ERROR('#4: var array = [,,,1,2]; array.length === 5. Actual: ' + (array.length));
}


if (array[0] !== undefined) {
  $ERROR('#5: var array = [,,,1,2]; array[0] === undefined. Actual: ' + (array[0]));
}


if (array[1] !== undefined) {
  $ERROR('#6: var array = [,,,1,2]; array[1] === undefined. Actual: ' + (array[1]));
}


if (array[2] !== undefined) {
  $ERROR('#7: var array = [,,,1,2]; array[2] === undefined. Actual: ' + (array[2]));
}


if (array[3] !== 1) {
  $ERROR('#8: var array = [,,,1,2]; array[3] === 1. Actual: ' + (array[3]));
}


if (array[4] !== 2) {
  $ERROR('#9: var array = [,,,1,2]; array[4] === 2. Actual: ' + (array[4]));
}


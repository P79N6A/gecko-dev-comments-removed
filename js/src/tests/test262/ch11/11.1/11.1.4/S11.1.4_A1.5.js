









var array = [4,5,,,,];


if (typeof array !== "object") {
  $ERROR('#1: var array = [4,5,,,,]; typeof array === "object". Actual: ' + (typeof array));
}


if (array instanceof Array !== true) {
  $ERROR('#2: var array = [4,5,,,,]; array instanceof Array === true');
}


if (array.toString !== Array.prototype.toString) {
  $ERROR('#3: var array = [4,5,,,,]; array.toString === Array.prototype.toString. Actual: ' + (array.toString));
}


if (array.length !== 5) {
  $ERROR('#4: var array = [4,5,,,,]; array.length === 5. Actual: ' + (array.length));
}


if (array[0] !== 4) {
  $ERROR('#5: var array = [4,5,,,,]; array[0] === 4. Actual: ' + (array[0]));
}


if (array[1] !== 5) {
  $ERROR('#6: var array = [4,5,,,,]; array[1] === 5. Actual: ' + (array[1]));
}


if (array[2] !== undefined) {
  $ERROR('#7: var array = [4,5,,,,]; array[2] === undefined. Actual: ' + (array[2]));
}


if (array[3] !== undefined) {
  $ERROR('#8: var array = [4,5,,,,]; array[3] === undefined. Actual: ' + (array[3]));
}


if (array[4] !== undefined) {
  $ERROR('#9: var array = [4,5,,,,]; array[4] === undefined. Actual: ' + (array[4]));
}


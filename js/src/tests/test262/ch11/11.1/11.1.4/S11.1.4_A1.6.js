









var array = [,,3,,,];


if (typeof array !== "object") {
  $ERROR('#1: var array = [,,3,,,]; typeof array === "object". Actual: ' + (typeof array));
}


if (array instanceof Array !== true) {
  $ERROR('#2: var array = [,,3,,,]; array instanceof Array === true');
}


if (array.toString !== Array.prototype.toString) {
  $ERROR('#3: var array = [,,3,,,]; array.toString === Array.prototype.toString. Actual: ' + (array.toString));
}


if (array.length !== 5) {
  $ERROR('#4: var array = [,,3,,,]; array.length === 5. Actual: ' + (array.length));
}


if (array[0] !== undefined) {
  $ERROR('#5: var array = [,,3,,,]; array[0] === undefined. Actual: ' + (array[0]));
}


if (array[1] !== undefined) {
  $ERROR('#6: var array = [,,3,,,]; array[1] === undefined. Actual: ' + (array[1]));
}


if (array[2] !== 3) {
  $ERROR('#7: var array = [,,3,,,]; array[2] === 3. Actual: ' + (array[2]));
}


if (array[3] !== undefined) {
  $ERROR('#8: var array = [,,3,,,]; array[3] === undefined. Actual: ' + (array[3]));
}


if (array[4] !== undefined) {
  $ERROR('#9: var array = [,,3,,,]; array[4] === undefined. Actual: ' + (array[4]));
}


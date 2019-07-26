









var array = [[1,2], [3], []];


if (typeof array !== "object") {
  $ERROR('#1: var array = [[1,2], [3], []]; typeof array === "object". Actual: ' + (typeof array));
}


if (array instanceof Array !== true) {
  $ERROR('#2: var array = [[1,2], [3], []]; array instanceof Array === true');
}


if (array.toString !== Array.prototype.toString) {
  $ERROR('#3: var array = [[1,2], [3], []]; array.toString === Array.prototype.toString. Actual: ' + (array.toString));
}


if (array.length !== 3) {
  $ERROR('#4: var array = [[1,2], [3], []]; array.length === 3. Actual: ' + (array.length));
}

var subarray = array[0];


if (typeof subarray !== "object") {
  $ERROR('#5: var array = [[1,2], [3], []]; var subarray = array[0]; typeof subarray === "object". Actual: ' + (typeof subarray));
}


if (subarray instanceof Array !== true) {
  $ERROR('#6: var array = [[1,2], [3], []]; var subarray = array[0]; subarray instanceof Array === true');
}


if (subarray.toString !== Array.prototype.toString) {
  $ERROR('#7: var array = [[1,2], [3], []]; var subarray = array[0]; subarray.toString === Array.prototype.toString. Actual: ' + (subarray.toString));
}


if (subarray.length !== 2) {
  $ERROR('#8: var array = [[1,2], [3], []]; var subarray = array[0]; subarray.length === 2. Actual: ' + (subarray.length));
}


if (subarray[0] !== 1) {
  $ERROR('#9: var array = [[1,2], [3], []]; var subarray = array[0]; subarray[0] === 1. Actual: ' + (subarray[0]));
}


if (subarray[1] !== 2) {
  $ERROR('#10: var array = [[1,2], [3], []]; var subarray = array[1]; subarray[1] === 2. Actual: ' + (subarray[1]));
}

var subarray = array[1];


if (typeof subarray !== "object") {
$ERROR('#11: var array = [[1,2], [3], []]; var subarray = array[1]; typeof subarray === "object". Actual: ' + (typeof subarray));
}


if (subarray instanceof Array !== true) {
$ERROR('#12: var array = [[1,2], [3], []]; var subarray = array[1]; subarray instanceof Array === true');
}


if (subarray.toString !== Array.prototype.toString) {
$ERROR('#13: var array = [[1,2], [3], []]; var subarray = array[1]; subarray.toString === Array.prototype.toString. Actual: ' + (subarray.toString));
}


if (subarray.length !== 1) {
$ERROR('#14: var array = [[1,2], [3], []]; var subarray = array[1]; subarray.length === 1. Actual: ' + (subarray.length));
}


if (subarray[0] !== 3) {
$ERROR('#15: var array = [[1,2], [3], []]; var subarray = array[1]; subarray[0] === 3. Actual: ' + (subarray[0]));
}

var subarray = array[2];


if (typeof subarray !== "object") {
$ERROR('#16: var array = [[1,2], [3], []]; var subarray = array[2]; typeof subarray === "object". Actual: ' + (typeof subarray));
}


if (subarray instanceof Array !== true) {
$ERROR('#17: var array = [[1,2], [3], []]; var subarray = array[2]; subarray instanceof Array === true');
}


if (subarray.toString !== Array.prototype.toString) {
$ERROR('#18: var array = [[1,2], [3], []]; var subarray = array[2]; subarray.toString === Array.prototype.toString. Actual: ' + (subarray.toString));
}


if (subarray.length !== 0) {
$ERROR('#19: var array = [[1,2], [3], []]; var subarray = array[2]; subarray.length === 0. Actual: ' + (subarray.length));
}


if (array[0][0] !== 1) {
  $ERROR('#20: var array = [[1,2], [3], []]; array[0][0] === 1. Actual: ' + (array[0][0]));
}


if (array[0][1] !== 2) {
  $ERROR('#21: var array = [[1,2], [3], []]; array[0][1] === 2. Actual: ' + (array[0][1]));
}


if (array[1][0] !== 3) {
  $ERROR('#722: var array = [[1,2], [3], []]; array[1][0] === 3. Actual: ' + (array[1][0]));
}


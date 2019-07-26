










if (this.x !== undefined) {
  $ERROR('#1: this.x === undefined. Actual: ' + (this.x));
}


var object = new Object();
if (object.prop !== undefined) {
  $ERROR('#2: var object = new Object(); object.prop === undefined. Actual: ' + (object.prop));
}


this.y++;
if (isNaN(y) !== true) {
  $ERROR('#3: this.y++; y === Not-a-Number. Actual: ' + (y));
}


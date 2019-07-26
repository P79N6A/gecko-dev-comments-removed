










var x = 1;
var y = x++;
if (y !== 1) {
  $ERROR('#1: var x = 1; var y = x++; y === 1. Actual: ' + (y));
} else {
  if (x !== 1 + 1) {
    $ERROR('#1: var x = 1; var y = x++; x === 1 + 1. Actual: ' + (x));
  } 
}


this.x = 1;
var y = this.x++; 
if (y !== 1) {
  $ERROR('#2: this.x = 1; var y = this.x++; y === 1. Actual: ' + (y));
} else {
  if (this.x !== 1 + 1) {
    $ERROR('#2: this.x = 1; var y = this.x++; this.x === 1 + 1. Actual: ' + (this.x));
  } 
}


var object = new Object();
object.prop = 1;
var y = object.prop++;
if (y !== 1) {
  $ERROR('#3: var object = new Object(); object.prop = 1; var y = object.prop++; y === 1. Actual: ' + (y));
} else {
  if (this.x !== 1 + 1) {
    $ERROR('#3: var object = new Object(); object.prop = 1; var y = object.prop++; object.prop === 1 + 1. Actual: ' + (object.prop));
  } 
}




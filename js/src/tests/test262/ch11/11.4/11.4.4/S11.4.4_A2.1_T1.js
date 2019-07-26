










var x = 1;
if (++x !== 1 + 1) {
  $ERROR('#1: var x = 1; ++x === 1 + 1. Actual: ' + (++x));
} else {
  if (x !== 1 + 1) {
    $ERROR('#1: var x = 1; ++x; x === 1 + 1. Actual: ' + (x));
  } 
}


this.x = 1;
if (++this.x !== 1 + 1) {
  $ERROR('#2: this.x = 1; ++this.x === 1 + 1. Actual: ' + (++this.x));
} else {
  if (this.x !== 1 + 1) {
    $ERROR('#2: this.x = 1; ++this.x; this.x === 1 + 1. Actual: ' + (this.x));
  } 
}


var object = new Object();
object.prop = 1;
if (++object.prop !== 1 + 1) {
  $ERROR('#3: var object = new Object(); object.prop = 1; ++object.prop === 1 + 1. Actual: ' + (++object.prop));
} else {
  if (this.x !== 1 + 1) {
    $ERROR('#3: var object = new Object(); object.prop = 1; ++object.prop; object.prop === 1 + 1. Actual: ' + (object.prop));
  } 
}


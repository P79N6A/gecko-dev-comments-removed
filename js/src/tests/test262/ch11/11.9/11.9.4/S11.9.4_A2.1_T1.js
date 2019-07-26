










if (!(1 === 1)) {
  $ERROR('#1: 1 === 1');
}


var x = 1;
if (!(x === 1)) {
  $ERROR('#2: var x = 1; x === 1');
}


var y = 1;
if (!(1 === y)) {
  $ERROR('#3: var y = 1; 1 === y');
}


var x = 1;
var y = 1;
if (!(x === y)) {
  $ERROR('#4: var x = 1; var y = 1; x === y');
}


var objectx = new Object();
var objecty = new Object();
objectx.prop = 1;
objecty.prop = 1;
if (!(objectx.prop === objecty.prop)) {
  $ERROR('#5: var objectx = new Object(); var objecty = new Object(); objectx.prop = 1; objecty.prop = 1; objectx.prop === objecty.prop');
}



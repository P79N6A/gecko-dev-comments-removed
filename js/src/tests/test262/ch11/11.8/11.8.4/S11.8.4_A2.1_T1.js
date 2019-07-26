










if (1 >= 1 !== true) {
  $ERROR('#1: 1 >= 1 === true');
}


var x = 1;
if (x >= 1 !== true) {
  $ERROR('#2: var x = 1; x >= 1 === true');
}


var y = 1;
if (1 >= y !== true) {
  $ERROR('#3: var y = 1; 1 >= y === true');
}


var x = 1;
var y = 1;
if (x >= y !== true) {
  $ERROR('#4: var x = 1; var y = 1; x >= y === true');
}


var objectx = new Object();
var objecty = new Object();
objectx.prop = 1;
objecty.prop = 1;
if (objectx.prop >= objecty.prop !== true) {
  $ERROR('#5: var objectx = new Object(); var objecty = new Object(); objectx.prop = 1; objecty.prop = 1; objectx.prop >= objecty.prop === true');
}













var x = 1;
if (delete x !== false) {
  $ERROR('#1: var x = 1; delete x === false');
}


var y = 1;
if (delete this.y !== false) {
  $ERROR('#2: var y = 1; delete this.y === false');
}


function MyFunction(){};
if (delete MyFunction !== false) {
  $ERROR('#3: function MyFunction(){}; delete MyFunction === false');
}


function MyFunction(){};
var MyObject = new MyFunction();
if (delete MyObject !== false) {
  $ERROR('#4: function MyFunction(){}; var MyObject = new MyFunction(); delete MyObject === false');
}


if (delete MyObject !== false) {
  $ERROR('#5: function MyFunction(){}; var MyObject = new MyFunction(); delete MyObject === false');
}


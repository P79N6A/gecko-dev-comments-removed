










try {
  x = 1;
  delete x;
  x;    
  $ERROR('#1: x = 1; delete x; x is not exist');
} catch (e) {
  if (e instanceof ReferenceError !== true) {
    $ERROR('#1: x = 1; delete x; x is not exist');
  }
}



function MyFunction(){};
MyFunction.prop = 1;
delete MyFunction.prop;
if (MyFunction.prop !== undefined) {
  $ERROR('#2: function MyFunction(){}; MyFunction.prop = 1; delete MyFunction.prop; MyFunction.prop === undefined. Actual: ' + (MyFunction.prop));

}


function MyFunction(){};
var MyObjectVar = new MyFunction();
MyObjectVar.prop = 1;
delete MyObjectVar.prop;
if (MyObjectVar.prop !== undefined) {
  $ERROR('#3: function MyFunction(){}; var MyObjectVar = new MyFunction(); MyFunction.prop = 1; delete MyObjectVar.prop; MyObjectVar.prop === undefined. Actual: ' + (MyObjectVar.prop));
}


if (delete MyObjectVar !== false) {
  $ERROR('#4: function MyFunction(){}; var MyObjectVar = new MyFunction(); delete MyObjectVar === false');
}


function MyFunction(){};
MyObjectNotVar = new MyFunction();
MyObjectNotVar.prop = 1;
delete MyObjectNotVar.prop;
if (MyObjectNotVar.prop !== undefined) {
  $ERROR('#5: function MyFunction(){}; MyObjectNotVar = new MyFunction(); MyFunction.prop = 1; delete MyObjectNotVar.prop; MyObjectNotVar.prop === undefined. Actual: ' + (MyObjectNotVar.prop));
}


if (delete MyObjectNotVar !== true) {
  $ERROR('#6: function MyFunction(){}; var MyObjectNotVar = new MyFunction(); delete MyObjectNotVar === true');
}















function MyFunction() {return this}
if (MyFunction() !== this) {
  $ERROR('#1: function MyFunction() {return this} MyFunction() === this. Actual: ' + (MyFunction()));
}


function MyFunction() {return eval("this")}
if (MyFunction() !== this) {
  $ERROR('#2: function MyFunction() {return eval("this")} MyFunction() === this. Actual: ' + (MyFunction()));
}




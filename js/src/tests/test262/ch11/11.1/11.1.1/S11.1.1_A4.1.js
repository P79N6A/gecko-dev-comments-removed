










var MyFunction = new Function("return this");
if (MyFunction() !== this) {
  $ERROR('#1: var MyFunction = new Function("return this"); MyFunction() === this. Actual: ' + (MyFunction()));
}


MyFunction = new Function("return eval(\'this\')");
if (MyFunction() !== this) {
  $ERROR('#2: var MyFunction = new Function("return eval(\'this\')"); MyFunction() === this. Actual: ' + (MyFunction()));
}




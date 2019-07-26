










var __obj={};



if (!(__obj instanceof Object)) {
  $ERROR('#1: var __obj={}; (__obj instanceof Object) === true. Actual: ' + ((__obj instanceof Object)));
}





if (__obj instanceof Function) {
  $ERROR('#2: var __obj={}; (__obj instanceof Function) === false. Actual: ' + ((__obj instanceof Function)));
}





if (__obj instanceof String) {
  $ERROR('#3: var __obj={}; (__obj instanceof String) === false. Actual: ' + ((__obj instanceof String)));
}





if (__obj instanceof Number) {
  $ERROR('#4: var __obj={}; (__obj instanceof Number) === false. Actual: ' + ((__obj instanceof Number)));
}





if (__obj instanceof Array) {
  $ERROR('#5: var __obj={}; (__obj instanceof Array) === false. Actual: ' + ((__obj instanceof Array)));
}




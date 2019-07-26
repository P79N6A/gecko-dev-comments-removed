









function __mFunc(){return arguments.length;};


if (__mFunc(1,2,3) !== 3){
  $ERROR('#1: function __mFunc(){return arguments.length;}; __mFunc(1,2,3) === 3. Actual: ' + (__mFunc(1,2,3)));
}




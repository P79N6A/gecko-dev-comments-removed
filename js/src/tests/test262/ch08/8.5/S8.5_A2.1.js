









var x = 9007199254740994.0; 
var y = 1.0 - 1/65536.0;
var z = x + y;
var d = z - x;

if (d !== 0){
  $ERROR('#1: var x = 9007199254740994.0; var y = 1.0 - 1/65536.0; var z = x + y; var d = z - x; d === 0. Actual: ' + (d));
}


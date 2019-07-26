










var x = 0;
var y = 0;
x
++y
if (x !== 0) {
  $ERROR('#1: Check Prefix Increment Operator for automatic semicolon insertion');
} else {
  if (y !== 1) {
    $ERROR('#2: Check Prefix Increment Operator for automatic semicolon insertion');
  }
}
 


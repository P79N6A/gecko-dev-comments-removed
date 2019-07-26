










try {
  var x=0;
}
catch (e) {
  $ERROR('#1: If Result(1).type is not throw, return Result(1). Actual: 4 Return(Result(3))');
}


var c1=0;
try{
  var x1=1;
}
finally
{
  c1=1;
}
if(x1!==1){
  $ERROR('#2.1: "try" block must be evaluated. Actual: try Block has not been evaluated');
}
if (c1!==1){
  $ERROR('#2.2: "finally" block must be evaluated. Actual: finally Block has not been evaluated');
}


var c2=0;
try{
  var x2=1;
}
catch(e){
  $ERROR('#3.1: If Result(1).type is not throw, return Result(1). Actual: 4 Return(Result(3))');	
}
finally{
  c2=1;
}
if(x2!==1){
  $ERROR('#3.2: "try" block must be evaluated. Actual: try Block has not been evaluated');
}
if (c2!==1){
  $ERROR('#3.3: "finally" block must be evaluated. Actual: finally Block has not been evaluated');
}


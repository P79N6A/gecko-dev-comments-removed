










try{
  y;
  $ERROR('#1: "y" lead to throwing exception');
}
catch(e){}


var c2=0;
try{
  try{
    someValue;
    $ERROR('#3.1: "someValues" lead to throwing exception');
  }
  finally{
    c2=1;
  }
}
catch(e){
  if (c2!==1){
    $ERROR('#3.2: "finally" block must be evaluated');
  }
}


var c3=0,x3=0;
try{
  x3=someValue;
  $ERROR('#3.1: "x3=someValues" lead to throwing exception');
}
catch(err){  	
  x3=1;
}
finally{
  c3=1;
}
if (x3!==1){
  $ERROR('#3.2: "catch" block must be evaluated');
}
if (c3!==1){
  $ERROR('#3.3: "finally" block must be evaluated');
}


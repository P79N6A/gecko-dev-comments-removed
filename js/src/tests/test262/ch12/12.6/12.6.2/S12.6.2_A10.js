









var check=0;
while(function f(){}){    
  if(typeof(f) === "function") {
    check = -1;
    break; 
  } else {
    check = 1;
    break; 
  }
}



if (check !== 1) {
	$ERROR('#1: FunctionExpression inside while construction expression allowed but function not declare');
}




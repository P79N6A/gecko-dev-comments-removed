











var x=function f1(){return 1;}();
if(x!==1)
  $ERROR('#1: Create function dynamically either by using a FunctionExpression');


var y=function  (){return 2;}();
if(y!==2){
  $ERROR('#2: Create an anonymous function dynamically either by using a FunctionExpression');
}


var z = (function(){return 3;})();
if(z!==3){
  $ERROR('#3: Create an anonymous function dynamically either by using a FunctionExpression wrapped in a group operator');
}














var objInstance=new Object;
if (objInstance.constructor !== Object){
  $ERROR('#1: var objInstance=new Object; objInstance.constructor === Object. Actual: ' + (objInstance.constructor));
}





var numInstance=new Number;
if (numInstance.constructor !== Number){
  $ERROR('#2: var numInstance=new Number; numInstance.constructor === Number. Actual: ' + (numInstance.constructor));
}




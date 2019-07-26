














var obj = new Object();


var objRef = obj;

objRef.oneProperty = -1;
obj.oneProperty = true;


if(objRef.oneProperty !== true){
  $ERROR('#1: var obj = new Object(); var objRef = obj; objRef.oneProperty = -1; obj.oneProperty = true; objRef.oneProperty === true. Actual: ' + (objRef.oneProperty));
};




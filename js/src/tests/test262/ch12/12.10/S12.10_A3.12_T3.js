











this.p1 = 1;
var result = "result";
var value = "value";
var myObj = {p1: 'a', 
             value: 'myObj_value',
             valueOf : function(){return 'obj_valueOf';}
}

with(myObj){
  var f = function(){
    return value;
    p1 = 'x1';
  }
}
result = f();

if(!(p1 === 1)){
  $ERROR('#1: p1 === 1. Actual:  p1 ==='+ p1  );
}

if(!(myObj.p1 === "a")){
  $ERROR('#2: myObj.p1 === "a". Actual:  myObj.p1 ==='+ myObj.p1  );
}

if(!(result === "myObj_value")){
  $ERROR('#3: result === "myObj_value". Actual:  result ==='+ result  );
}


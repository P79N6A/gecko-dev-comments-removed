











this.p1 = 1;
var result = "result";
var value = "value";
var myObj = {p1: 'a', 
             value: 'myObj_value',
             valueOf : function(){return 'obj_valueOf';}
}

try {
  var f = function(){
    p1 = 'x1';
    throw value;
  }
  
  with(myObj){
    f();
  }
} catch(e){
  result = e;
}

if(!(p1 === "x1")){
  $ERROR('#1: p1 === "x1". Actual:  p1 ==='+ p1  );
}

if(!(myObj.p1 === "a")){
  $ERROR('#2: myObj.p1 === "a". Actual:  myObj.p1 ==='+ myObj.p1  );
}

if(!(result === "value")){
  $ERROR('#3: result === "value". Actual:  result ==='+ result  );
}




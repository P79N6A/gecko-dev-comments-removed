











this.p1 = 1;

var result = "result";

var myObj = {
    p1: 'a', 
    value: 'myObj_value',
    valueOf : function(){return 'obj_valueOf';}
}

try {
    with(myObj){
        (function f(){
            throw value;
            p1 = 'x1';
        })();
    }
} catch(e){
    result = p1;
}



if(result !== 1){
  $ERROR('#1: result === 1. Actual:  result ==='+ result  );
}





if(p1 !== 1){
  $ERROR('#2: p1 === 1. Actual:  p1 ==='+ p1  );
}





if(myObj.p1 !== "a"){
  $ERROR('#3: myObj.p1 === "a". Actual:  myObj.p1 ==='+ myObj.p1  );
}




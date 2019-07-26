











this.p1 = 1;

var result = "result";

var myObj = {
    p1: 'a', 
    value: 'myObj_value',
    valueOf : function(){return 'obj_valueOf';}
}

with(myObj){
    result=(function(){
        p1 = 'x1';
        return value;
    })();
}



if(p1 !== 1){
  $ERROR('#1: p1 === 1. Actual:  p1 ==='+ p1  );
}





if(result !== "myObj_value"){
  $ERROR('#2: result === "myObj_value". Actual:  result ==='+ result  );
}





if(myObj.p1 !== "x1"){
  $ERROR('#3: myObj.p1 === "x1". Actual:  myObj.p1 ==='+ myObj.p1  );
}






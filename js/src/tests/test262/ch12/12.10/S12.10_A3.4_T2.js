











this.p1 = 1;

var result = "result";

var myObj = {
    p1: 'a', 
    value: 'myObj_value',
    valueOf : function(){return 'obj_valueOf';}
}

try {
    do{
        with(myObj){
            p1 = 'x1';
            throw value;
        }
    } while(false);
} catch(e){
    result = p1;
}



if(result !== 1){
  $ERROR('#1: result === 1. Actual:  result ==='+ result  );
}





if(p1 !== 1){
  $ERROR('#2: p1 === 1. Actual:  p1 ==='+ p1  );
}





if(myObj.p1 !== "x1"){
  $ERROR('#3: myObj.p1 === "x1". Actual:  myObj.p1 ==='+ myObj.p1  );
}




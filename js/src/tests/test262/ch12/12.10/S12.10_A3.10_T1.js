











this.p1 = 1;

var result = "result";

var myObj = {
    p1: 'a', 
    value: 'myObj_value',
    valueOf : function(){return 'obj_valueOf';}
}

with(myObj){
    do{
        p1 = 'x1';
    } while(false);
}



if(p1 !== 1){
  $ERROR('#1: p1 === 1. Actual:  p1 ==='+ p1  );
}





if(myObj.p1 !== "x1"){
  $ERROR('#2: myObj.p1 === "x1". Actual:  myObj.p1 ==='+ myObj.p1  );
}







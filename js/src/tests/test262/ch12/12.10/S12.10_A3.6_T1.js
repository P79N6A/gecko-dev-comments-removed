











this.p1 = 1;

var result = "result";

var myObj = {
    p1: 'a', 
    value: 'myObj_value',
    valueOf : function(){return 'obj_valueOf';}
}

var theirObj = {
    p1: true, 
    value: 'theirObj_value',
    valueOf : function(){return 'thr_valueOf';}
}

with(myObj){
    with(theirObj){
        p1 = 'x1';
  }
}



if(p1 !== 1){
  $ERROR('#1: p1 === 1. Actual:  p1 ==='+ p1  );
}





if(myObj.p1 !== "a"){
  $ERROR('#2: myObj.p1 === "a". Actual:  myObj.p1 ==='+ myObj.p1  );
}





if(theirObj.p1 !== "x1"){
  $ERROR('#3: theirObj.p1 === "x1". Actual:  theirObj.p1 ==='+ theirObj.p1  );
}






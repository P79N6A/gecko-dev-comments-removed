











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


try {
    with(myObj){
        with(theirObj){
            throw value;
            p1 = 'x1';
            
        }
    }
} catch(e){
    result = p1;
}



if(p1 !== 1){
  $ERROR('#1: p1 === 1. Actual:  p1 ==='+ p1  );
}





if(myObj.p1 !== "a"){
  $ERROR('#2: myObj.p1 === "a". Actual:  myObj.p1 ==='+ myObj.p1  );
}





if(theirObj.p1 !== true){
  $ERROR('#3: theirObj.p1 === true. Actual:  theirObj.p1 ==='+ theirObj.p1  );
}
















p1="alert";

this.__obj={p1:1,getRight:function(){return "right";}};

var getRight=function(){return "napravo";};

resukt=(function(){
    with(__obj){
        p1="w1";
        var getRight=function(){return false;};
        return p1;
    }
})();



if (p1!=="alert") {
	$ERROR('#1: p1 === "alert". Actual: p1==='+p1);
}





if (getRight()!=="napravo") {
	$ERROR('#2: getRight() === "napravo". Actual: getRight()==='+getRight());
}





if (__obj.p1!=="w1") {
	$ERROR('#3: __obj.p1 === "w1". Actual: __obj.p1 ==='+__obj.p1);
}





if (__obj.getRight()!==false) {
	$ERROR('#4: __obj.getRight() === false. Actual: __obj.getRight()==='+__obj.getRight());
}





if (resukt !== "w1") {
	$ERROR('#5: resukt === "w1". Actual: resukt ==='+resukt);
}



var resukt;













this.p1="alert";

__obj={p1:1,getRight:function(){return "right";}};

getRight=function(){return "napravo";};

try {
	(function(){
        with(__obj){
            p1="w1";
            getRight=function(){return false;}
            throw p1;
        }
    })();
} catch (e) {
	resukt = p1;
}




if (p1!=="alert") {
	$ERROR('#1: p1 === "alert". Actual: p1==='+p1);
}





if (getRight()!=="napravo") {
	$ERROR('#2: getRight() === "napravo". Actual: getRight() === '+getRight());
}





if (__obj.p1!=="w1") {
	$ERROR('#3: __obj.p1 === "w1". Actual: __obj.p1 ==='+__obj.p1);
}





if (__obj.getRight()!==false) {
	$ERROR('#4: __obj.getRight() === false. Actual: __obj.getRight() === '+__obj.getRight());
}





if (resukt !== "alert") {
	$ERROR('#5: resukt === "alert". Actual: resukt ==='+resukt);
}



var resukt;













this.callee = 0;
var b;

__obj={callee:"a"};

function f(){
    with (arguments){
        callee=1;
        b=true;
        return arguments;
    }
};

result=f(__obj);



if (callee !== 0) {
	$ERROR('#1: callee === 0. Actual: callee ==='+callee);
}





if (__obj.callee !== "a") {
	$ERROR('#2: __obj.callee === "a". Actual: __obj.callee ==='+__obj.callee);
}





if (result.callee !== 1) {
	$ERROR('#3: result.callee === 1. Actual: result.callee ==='+result.callee);
}





if (!(this.b)) {
	$ERROR('#4: this.b === true. Actual: this.b ==='+this.b);
}




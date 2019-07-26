










var x; 

function __func(){
    x = 1;
    return;
}



if (__func() !== undefined) {
	$ERROR('#1: __func() === undefined. Actual: __func() ==='+__func());
};





if (x!==1) {
	$ERROR('#2: x === 1. Actual: x === '+x);
}




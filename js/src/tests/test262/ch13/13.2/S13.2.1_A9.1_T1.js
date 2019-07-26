










var x;

function __func(){
    x = true;
}



if (__func() !== undefined) {
	$ERROR('#1: __func() === undefined. Actual: __func() ==='+__func());
};





if (!x) {
	$ERROR('#2: x === true. Actual: x === '+x);
}




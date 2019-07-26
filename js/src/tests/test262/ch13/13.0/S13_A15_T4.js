









THE_ANSWER="Answer to Life, the Universe, and Everything";

function __func(){
    return typeof arguments;
    var arguments = THE_ANSWER;
};



if (__func(42,42,42) !== "object") {
	$ERROR('#1: __func(42,42,42) === "object". Actual: __func(42,42,42)==='+__func(42,42,42));
}




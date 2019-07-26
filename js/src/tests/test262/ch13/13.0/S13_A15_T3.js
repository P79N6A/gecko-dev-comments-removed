









THE_ANSWER="Answer to Life, the Universe, and Everything";

var arguments = THE_ANSWER;

function __func(arguments){
    return arguments;
    
};



if (typeof __func() !== "undefined") {
	$ERROR('#1: typeof __func() === "undefined". Actual: typeof __func() ==='+typeof __func());
}





if (__func("The Ultimate Question") !== "The Ultimate Question") {
	$ERROR('#2: __func("The Ultimate Question") === "The Ultimate Question". Actual: __func("The Ultimate Question")==='+__func("The Ultimate Question"));
}




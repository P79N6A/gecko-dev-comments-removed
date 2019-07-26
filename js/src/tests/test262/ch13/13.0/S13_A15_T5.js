









THE_ANSWER="Answer to Life, the Universe, and Everything";

var arguments = THE_ANSWER;

function __func(){
    return arguments;
};



if ( __func() === THE_ANSWER) {
	$ERROR('#1: __func() !== THE_ANSWER');
}





if (__func("The Ultimate Question") === "The Ultimate Question") {
	$ERROR('#2: __func("The Ultimate Question") !== "The Ultimate Question"');
}




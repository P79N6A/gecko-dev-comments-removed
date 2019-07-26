









THE_ANSWER="Answer to Life, the Universe, and Everything";

function __func(){
    var arguments = THE_ANSWER;
    return arguments;
};



if (__func(42,42,42) !== THE_ANSWER) {
	$ERROR('#1:  "arguments" variable overrides ActivationObject.arguments');
}




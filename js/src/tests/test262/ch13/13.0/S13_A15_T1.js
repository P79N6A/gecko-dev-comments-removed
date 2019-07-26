









function __func(arguments){
    return arguments;
};



if (__func(42) !== 42) {
	$ERROR('#1: "arguments" variable overrides ActivationObject.arguments');
}




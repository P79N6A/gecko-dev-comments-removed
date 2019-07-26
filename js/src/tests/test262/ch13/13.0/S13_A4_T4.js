









function __func(){return arguments[0].name + " " + arguments[0].surname;};



if (typeof __func !== "function") {
	$ERROR('#1: typeof __func === "function". Actual: typeof __func ==='+typeof __func);
}





if (__func({name:'fox', surname:'malder'}) !== "fox malder") {
	$ERROR('#2: __func({name:\'fox\', surname:\'malder\'}) === "fox malder". Actual: __func({name:\'fox\', surname:\'malder\'}) ==='+__func({name:'fox', surname:'malder'}));
}



function func__(arg){return arg.name + " " + arg.surname;};



if (typeof func__ !== "function") {
	$ERROR('#3: typeof func__ === "function". Actual: typeof __func ==='+typeof __func);
}





if (func__({name:'john', surname:'lennon'}) !== "john lennon") {
	$ERROR('#4: func__({name:\'john\', surname:\'lennon\'}) === "john lennon". Actual: __func({name:\'john\', surname:\'lennon\'}) ==='+__func({name:'john', surname:'lennon'}));
}




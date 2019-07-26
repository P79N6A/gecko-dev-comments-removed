









function FACTORY(){this.prop=1;this.hint="hinted"};

FACTORY.prototype = {feat:2,hint:"protohint"};

var __instance = new FACTORY;

__accum="";

for (var key in __instance){
	__accum+=(key + __instance[key]);
}



if (!((__accum.indexOf("prop1")!==-1)&&(__accum.indexOf("feat2")!==-1)&&(__accum.indexOf("hinthinted")!==-1))) {
	$ERROR('#1: (__accum.indexOf("prop1")!==-1)&&(__accum.indexOf("feat2")!==-1)&&(__accum.indexOf("hinthinted")!==-1)');
}





if (__accum.indexOf("hintprotohint")!==-1) {
	$ERROR('#2: __accum.indexOf("hintprotohint") === -1. Actual:  __accum.indexOf("hintprotohint") ==='+ __accum.indexOf("hintprotohint")  );
}




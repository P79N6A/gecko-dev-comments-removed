










var __obj = new __FACTORY();



if (typeof obj !== "undefined") {
	$ERROR('#1: typeof obj === "undefined". Actual: typeof obj ==='+typeof obj);
}





if (__obj.prop !== "A") {
	$ERROR('#2: __obj.prop === "A". Actual: __obj.prop ==='+__obj.prop);
}





if (__obj.slot.prop !==1) {
	$ERROR('#3: __obj.slot.prop ===1. Actual: __obj.slot.prop ==='+__obj.slot.prop);
}



function __FACTORY(){
    this.prop = 1;
    var obj = {};
    obj.prop = "A";
    obj.slot = this;
    return obj;
}


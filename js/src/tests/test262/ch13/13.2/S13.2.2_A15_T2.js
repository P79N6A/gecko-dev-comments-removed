










var __obj = new __FACTORY();



if (obj.prop !== "A") {
	$ERROR('#1: obj.prop === "A". Actual: obj.prop ==='+obj.prop);
}





if (__obj.prop !== "A") {
	$ERROR('#2: __obj.prop === "A". Actual: __obj.prop ==='+__obj.prop);
}





if (__obj.slot.prop !==1) {
	$ERROR('#3: __obj.slot.prop === 1. Actual: __obj.slot.prop ==='+__obj.slot.prop);
}



function __FACTORY(){
    this.prop = 1;
    obj = {};
    obj.prop = "A";
    obj.slot = this;
    return obj;
}


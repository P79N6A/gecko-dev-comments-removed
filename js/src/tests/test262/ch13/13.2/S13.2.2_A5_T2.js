












__VOLUME=8;
__RED="red";
__ID=12342;
__TOP=1.1;
__BOTTOM=0.0;
__LEFT=0.0;


__FACTORY = function(arg1, arg2){
	this.volume=__VOLUME;
	color=__RED;
	this.id=arg1;
	top=arg2;
	this.bottom=arguments[3];
	left=arguments[4];
};

__device = new __FACTORY(__ID, __TOP, __BOTTOM, __LEFT);



if (__device.color !== undefined) {
	$ERROR('#1: __device.color === undefined. Actual: __device.color ==='+__device.color);
}





if (__device.volume !== __VOLUME) {
	$ERROR('#2: __device.volume === __VOLUME. Actual: __device.volume ==='+__device.volume);
}





if (__device.top !== undefined) {
	$ERROR('#3: __device.top === undefined. Actual: __device.top ==='+__device.top);
}





if (__device.id !== __ID) {
	$ERROR('#4: __device.id === __ID. Actual: __device.id ==='+__device.id);
}





if (__device.left !== undefined) {
	$ERROR('#5: __device.left === undefined. Actual: __device.left ==='+__device.left);
}





if (__device.bottom !== __BOTTOM) {
	$ERROR('#6: __device.bottom === __BOTTOM. Actual: __device.bottom ==='+__device.bottom);
}




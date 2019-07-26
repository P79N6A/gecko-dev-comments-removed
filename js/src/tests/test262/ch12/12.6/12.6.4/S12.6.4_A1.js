











try {
	for(__key in undefined){
	    var key=__key;
	};
} catch (e) {
	$ERROR('#1: "for(key in undefined){}" does not lead to throwing exception');
}






if (key!==undefined) {
	$ERROR('#2: key === undefined. Actual: key === '+key);
}






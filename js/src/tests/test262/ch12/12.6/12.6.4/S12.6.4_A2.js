











try {
	for(__key in null){
	    var key=__key;
	};
} catch (e) {
	$ERROR('#1: "for(__key in null){}" does not lead to throwing exception');
}





if (key!==undefined) {
	$ERROR('#2: key === undefined. Actual: key ==='+key);
}

















var a = 1;

var __obj = {a:2};

try {
	with (__obj)
    {
        var __func = function (){return a;};
        throw 3;
    }
} catch (e) {
	;
}

result = __func();



if (result !== 2) {
	$ERROR('#1: result === 2. Actual: result ==='+result);
}







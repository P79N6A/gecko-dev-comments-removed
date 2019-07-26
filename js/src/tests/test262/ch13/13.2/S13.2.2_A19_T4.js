









var a = 1;

var __obj = {a:2,__obj:{a:3}};

try {
	with (__obj)
    {
        with(__obj){
            var __func = function(){return a;};
            throw 5;
        }
    }
} catch (e) {
	;
}

result = __func();



if (result !== 3) {
	$ERROR('#1: result === 3. Actual: result ==='+result);
}

















var a = 1;

var __obj = {a:2};

with (__obj)
{
    try {
        
    	var __func = function()
        {
            return a;
        }
        throw 3;
    } catch (e) {
    	;
    }
}

delete __obj;

result = __func();



if (result !== 2) {
	$ERROR('#1: result === 2. Actual: result ==='+result);
}




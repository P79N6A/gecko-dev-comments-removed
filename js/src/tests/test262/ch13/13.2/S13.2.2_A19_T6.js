









var a = 1;

var __obj = {a:2};

with (__obj)
{
    do {
        var __func = function()
        {
            return a;
        }
    } while(0);
}

delete __obj;

var __obj = {a:3};

with (__obj)
{
    result = __func();
}



if (result !== 2) {
	$ERROR('#1: result === 2. Actual: result ==='+result);
}









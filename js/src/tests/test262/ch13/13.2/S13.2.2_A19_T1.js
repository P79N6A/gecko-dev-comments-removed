









var a = 1;

var __func= function(){return a;};

var __obj = {a:2};

with (__obj)
{
    result = __func();
}



if (result !== 1) {
	$ERROR('#1: result === 1. Actual: result ==='+result);
}




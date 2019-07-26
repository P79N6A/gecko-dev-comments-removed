











if (typeof __func !== "undefined") {
	$ERROR('#0: typeof __func === "undefined". Actual: typeof __func ==='+typeof __func);
}



var a = 1, b = "a";

var __obj = {a:2};

with (__obj)
{
    while(1){
        var  __func = function()
        {
            return a;
        };
        break;
    }
}

delete __obj;



if (__func() !== 2) {
	$ERROR('#1: __func() === 2. Actual: __func() ==='+__func());
}



var __obj = {a:3,b:"b"};

with (__obj)
{
    var __func = function()
    {
        return b;
    }
}

delete __obj;



if (__func()!=="b") {
	$ERROR('#2: __func()==="b". Actual: __func()==='+__func());
}



with ({a:99,b:"c"})
{
    
    
    if (__func() !== "b") {
    	$ERROR('#3: __func()==="b". Actual: __func()==='+__func());
    }
    
    
}


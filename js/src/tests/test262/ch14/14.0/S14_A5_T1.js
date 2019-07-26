











if (__func() !== "both") {
	$ERROR('#1: __func() === "both". Actual:  __func() ==='+ __func()  );
}



function __func(){return "ascii"};
function \u005f\u005f\u0066\u0075\u006e\u0063(){return "unicode"};
function __\u0066\u0075\u006e\u0063(){return "both"};


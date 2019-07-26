











if (__func() !== "unicode") {
	$ERROR('#1: __func() === "unicode". Actual:  __func() ==='+ __func()  );
}



function __func(){return "ascii"};
function \u005f\u005f\u0066\u0075\u006e\u0063(){return "unicode"};













































test();

function test()
{
	try
	{
		var variantObj = Components.classes["@mozilla.org/variant;1"].createInstance(Components.interfaces.nsIVariant);
		var jsobj = 
		{
			Boolean : false,
			Short : 0,
			Long : 0,
			Float : 0.0,
			Double : 0.0,
			Currency : 0.0,
			Date : new Date(),
			String : "",
			DispatchPtr : {},
			SCode : 0,
			Variant : variantObj,
			Char : 'a'
		};
		var obj = COMObject(CLSID_nsXPCDispTestWrappedJS);
		reportCompare(
			obj.TestParamTypes(jsobj),
			"",
			"Testing IDispatch wrapped JS objects");
	}
	catch (e)
	{
		reportFailure(e.toString());
	}
}
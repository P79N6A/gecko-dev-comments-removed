









































try
{
	
    testObject = { one : 1, two : 2, three : 'three' };
    test();
}
catch (e)
{
    reportFailure("Unhandled exception encountered: " + e.toString());
}




function test()
{
    printStatus("Read-Write Attributes");

    



    o = COMObject(CLSID_nsXPCDispTestProperties);

    
    reportCompare(
        "object",
        typeof o,
        "typeof COMObject(CLSID_nsXPCDispTestProperties)");
    
    testProperty("o.Boolean", ["true", "1","testObject", "'T'", "'F'", "'true'","'false'"], false);
    testProperty("o.Char", ["32"], true);
    testProperty("o.COMPtr", ["0"], true);
    testProperty("o.Currency", ["0", "0.5", "10000.00"], true);
    testProperty("o.Date", ["'04/01/03'"], false);
    testProperty("o.DispatchPtr", ["testObject"], false);
    testProperty("o.Double", ["5.555555555555555555555", "5.555555555555555555555", "5", "-5"], true);
    testProperty("o.Float", ["-5.55555555", "5.5555555", "5", "-5"], true);
    testProperty("o.Long", ["-5", "1073741823", "-1073741824", "1073741824", "-1073741825", "5.5"], true);
    testProperty("o.SCode", ["5"], true);
    testProperty("o.Short", ["5", "-5", "32767", "-32768"], true);
    testProperty("o.String", ["'A String'", "'c'", "5", "true"], false);
    testProperty("o.Variant", ["'A Variant String'", "testObject", "10", "5.5"], false);
    
    
    for (index = 0; index < o.ParameterizedPropertyCount; ++index)
        compareExpression(
            "o.ParameterizedProperty(index)",
            index + 1,
            "Reading initial value from parameterized property " + index);
    for (index = 0; index < o.ParameterizedPropertyCount; ++index)
        compareExpression(
            "o.ParameterizedProperty(index) = index + 5",
            index + 5,
            "Assigning parameterized property " + index);

    for (index = 0; index < o.ParameterizedPropertyCount; ++index)
        compareExpression(
            "o.ParameterizedProperty(index)",
            index + 5,
            "Reading new value from parameterized property " + index);
}





function testProperty(propertyExpression, values, tryString)
{
    print(propertyExpression);
    try
    {
        reportCompare(
            eval(propertyExpression),
            eval(propertyExpression),
            propertyExpression);
    }
    catch (e)
    {
        reportFailure("Testing initial value of " + propertyExpression + " Exception: " + e.toString());
    }
    for (i = 0; i < values.length; ++i)
    {
        var value = values[i];
        var expression = propertyExpression + "=" + value;
        print(expression);
        try
        {
            reportCompare(
                eval(expression),
                eval(value),
                expression);
            if (tryString)
            {
                expression = propertyExpression + "='" + value + "'";
                print(expression);
                reportCompare(
                    eval(expression),
                    eval("'" + value + "'"),
                    expression);
            }
        }
        catch (e)
        {
            reportFailure("Testing assignment: " + expression + " Exception: " + e.toString());
        }
    }
}
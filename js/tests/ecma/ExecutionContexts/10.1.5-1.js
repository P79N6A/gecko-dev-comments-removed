


























































    var SECTION = "10.5.1-1";
    var VERSION = "ECMA_1";
    startTest();

    writeHeaderToLog( SECTION + " Global Object");


    new TestCase( "SECTION", "Global Code check" );

    if ( Object == null ) {
        testcases[0].reason += " Object == null" ;
    }
    if ( Function == null ) {
        testcases[0].reason += " Function == null";
    }
    if ( String == null ) {
        testcases[0].reason += " String == null";
    }
    if ( Array == null ) {
        testcases[0].reason += " Array == null";
    }
    if ( Number == null ) {
        testcases[0].reason += " Function == null";
    }
    if ( Math == null ) {
        testcases[0].reason += " Math == null";
    }
    if ( Boolean == null ) {
        testcases[0].reason += " Boolean == null";
    }
    if ( Date  == null ) {
        testcases[0].reason += " Date == null";
    }








    if ( eval == null ) {
        testcases[0].reason += " eval == null";
    }
    if ( parseInt == null ) {
        testcases[0].reason += " parseInt == null";
    }

    if ( testcases[0].reason != "" ) {
        testcases[0].actual = "fail";
    } else {
        testcases[0].actual = "pass";
    }
    testcases[0].expect = "pass";

    test();


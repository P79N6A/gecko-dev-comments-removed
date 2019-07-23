




package com.netscape.javascript.qa.drivers;












public class TestCase {
    public String passed;
    public String name;
    public String description;
    public String expect;
    public String actual;
    public String reason;
    
    














    public TestCase( String passed, String name, String description,
                      String expect, String actual, String reason ) {

        this.passed = passed;
        this.name   = name;
        this.description = description;
        this.expect = expect;
        this.actual = actual;
        this.reason = reason;
    }
}

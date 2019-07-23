




package com.netscape.javascript.qa.drivers;

import java.io.*;
import java.util.*;














public class TestSuite extends Vector {
    public String  name;
    public String  filePath;
    public boolean passed;
    public int     totalCases;
    public int     casesPassed;
    public int     casesFailed;

    






    public TestSuite( String name, String filePath ) {
        this.name       = name;
        this.filePath   = filePath;
        this.passed     = true;
        this.totalCases  = 0;
        this.casesPassed = 0;
        this.casesFailed = 0;
    }
}
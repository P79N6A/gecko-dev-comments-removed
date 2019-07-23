




package com.netscape.javascript.qa.drivers;

import java.io.*;
import java.util.*;













public class TestFile extends File {
    public String   description;
    public String   name;
    public String   filePath;
    public boolean  passed;
    public boolean  completed;
    public String   startTime;
    public String   endTime;
    public String   reason;
    public String   program;
    public int      totalCases;
    public int      casesPassed;
    public int      casesFailed;
    public Vector   caseVector;
    public String   exception;
    public String   bugnumber;

    






    public TestFile( String name, String filePath ) {
        super( filePath );

        this.name       = name;
        this.filePath   = filePath;
        this.passed     = true;
        this.completed  = false;
        this.startTime  = "00:00:00";
        this.endTime    = "00:00:00";
        this.reason     = "";
        this.program    = "";
        this.totalCases  = 0;
        this.casesPassed = 0;
        this.casesFailed = 0;
        this.caseVector = new Vector();
        this.exception  = "";
        this.bugnumber = "";
     }
}
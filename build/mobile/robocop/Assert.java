



package org.mozilla.gecko;

public interface Assert {
    void dumpLog(String message);
    void dumpLog(String message, Throwable t);
    void setLogFile(String filename);
    void setTestName(String testName);
    void endTest();

    void ok(boolean condition, String name, String diag);
    void is(Object a, Object b, String name);
    void isnot(Object a, Object b, String name);
    void todo(boolean condition, String name, String diag);
    void todo_is(Object a, Object b, String name);
    void todo_isnot(Object a, Object b, String name);
    void info(String name, String message);

    
    void ispixel(int actual, int r, int g, int b, String name);
    void isnotpixel(int actual, int r, int g, int b, String name);
}

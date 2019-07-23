




package com.netscape.javascript.qa.drivers;













public interface TestEnvironment {
    


    
    public void runTest();

    






    public Object createContext();
    
    







    public Object executeTestFile();
    
    








    
    public boolean parseResult();

    


    public void close();
}
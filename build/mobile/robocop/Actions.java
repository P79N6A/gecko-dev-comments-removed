



package org.mozilla.gecko;
import android.database.Cursor;

public interface Actions {

    
    public enum SpecialKey {
        DOWN,
        UP,
        LEFT,
        RIGHT,
        ENTER,
        MENU,
        


        @Deprecated
        BACK
    }

    public interface EventExpecter {
        
        public void blockForEvent();
        public void blockForEvent(long millis, boolean failOnTimeout);

        
        public String blockForEventData();

        



        public String blockForEventDataWithTimeout(long millis);

        
        public boolean eventReceived();

        
        public void unregisterListener();
    }

    public interface RepeatedEventExpecter extends EventExpecter {
        
        public void blockUntilClear(long millis);
    }

    




    void sendGeckoEvent(String geckoEvent, String data);

    





    void sendPreferencesGetEvent(int requestId, String[] prefNames);

    





    void sendPreferencesObserveEvent(int requestId, String[] prefNames);

    




    void sendPreferencesRemoveObserversEvent(int requestid);

    






    RepeatedEventExpecter expectGeckoEvent(String geckoEvent);

    





    RepeatedEventExpecter expectPaint();

    




    void sendKeys(String keysToSend);

    




    void sendSpecialKey(SpecialKey key);
    void sendKeyCode(int keyCode);

    void drag(int startingX, int endingX, int startingY, int endingY);

    


    public Cursor querySql(String dbPath, String sql);
}

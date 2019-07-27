




package org.mozilla.gecko;

public class SmsManager {
    private static final ISmsManager sInstance;
    static {
        if (AppConstants.MOZ_WEBSMS_BACKEND) {
            sInstance = new GeckoSmsManager();
        } else {
            sInstance = null;
        }
    }

    public static ISmsManager getInstance() {
        return sInstance;
    }

    public static boolean isEnabled() {
        return AppConstants.MOZ_WEBSMS_BACKEND;
    }

    public interface ISmsManager {
        public void start();
        public void stop();
        public void shutdown();

        public void send(String aNumber, String aMessage, int aRequestId);
        public void getMessage(int aMessageId, int aRequestId);
        public void deleteMessage(int aMessageId, int aRequestId);
        public void createMessageList(long aStartDate, long aEndDate, String[] aNumbers, int aNumbersCount, String aDelivery, boolean aHasRead, boolean aRead, long aThreadId, boolean aReverse, int aRequestId);
        public void getNextMessageInList(int aListId, int aRequestId);
        public void clearMessageList(int aListId);
    }
}


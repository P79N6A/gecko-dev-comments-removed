




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
        void start();
        void stop();
        void shutdown();

        void send(String aNumber, String aMessage, int aRequestId);
        void getMessage(int aMessageId, int aRequestId);
        void deleteMessage(int aMessageId, int aRequestId);
        void createMessageList(long aStartDate, long aEndDate, String[] aNumbers, int aNumbersCount, String aDelivery, boolean aHasRead, boolean aRead, long aThreadId, boolean aReverse, int aRequestId);
        void getNextMessageInList(int aListId, int aRequestId);
        void clearMessageList(int aListId);
    }
}


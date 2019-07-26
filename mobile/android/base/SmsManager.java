




package org.mozilla.gecko;

class SmsManager
{
    static private ISmsManager sInstance = null;

    static public ISmsManager getInstance() {
        if (AppConstants.MOZ_WEBSMS_BACKEND) {
            if (sInstance == null) {
                sInstance = new GeckoSmsManager();
            }
        }
        return sInstance;
    }
}

interface ISmsManager
{
  public void start();
  public void stop();
  public void shutdown();

  public void send(String aNumber, String aMessage, int aRequestId);
  public void getMessage(int aMessageId, int aRequestId);
  public void deleteMessage(int aMessageId, int aRequestId);
  public void createMessageList(long aStartDate, long aEndDate, String[] aNumbers, int aNumbersCount, int aDeliveryState, boolean aReverse, int aRequestId);
  public void getNextMessageInList(int aListId, int aRequestId);
  public void clearMessageList(int aListId);
}

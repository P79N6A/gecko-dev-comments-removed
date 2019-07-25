




































package org.mozilla.gecko;

import java.util.ArrayList;

import android.util.Log;

import android.telephony.SmsManager;

public class GeckoSmsManager
{
  final static int kMaxMessageSize = 160;

  public static int getNumberOfMessagesForText(String aText) {
    return SmsManager.getDefault().divideMessage(aText).size();
  }

  public static void send(String aNumber, String aMessage) {
    




    try {
      SmsManager sm = SmsManager.getDefault();

      if (aMessage.length() <= kMaxMessageSize) {
        sm.sendTextMessage(aNumber, "", aMessage, null, null);
      } else {
        ArrayList<String> parts = sm.divideMessage(aMessage);
        sm.sendMultipartTextMessage(aNumber, "", parts, null, null);
      }
    } catch (Exception e) {
      Log.i("GeckoSmsManager", "Failed to send an SMS: ", e);
    }
  }
}

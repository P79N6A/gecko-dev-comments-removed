




































package org.mozilla.gecko;

import java.util.ArrayList;

import android.util.Log;

import android.telephony.SmsManager;

public class GeckoSmsManager
{
  public static int getNumberOfMessagesForText(String aText) {
    return SmsManager.getDefault().divideMessage(aText).size();
  }
}






































package org.mozilla.gecko;

import java.util.ArrayList;

import android.util.Log;

import android.content.Intent;
import android.content.BroadcastReceiver;
import android.content.Context;

import android.os.Bundle;

import android.telephony.SmsManager;
import android.telephony.SmsMessage;

public class GeckoSmsManager
  extends BroadcastReceiver
{
  final static int kMaxMessageSize = 160;

  @Override
  public void onReceive(Context context, Intent intent) {
    if (intent.getAction().equals("android.provider.Telephony.SMS_RECEIVED")) {
      
      
      
      
      

      Bundle bundle = intent.getExtras();

      if (bundle == null) {
        return;
      }

      Object[] pdus = (Object[]) bundle.get("pdus");

      for (int i=0; i<pdus.length; ++i) {
        SmsMessage msg = SmsMessage.createFromPdu((byte[])pdus[i]);

        GeckoAppShell.notifySmsReceived(msg.getDisplayOriginatingAddress(),
                                        msg.getDisplayMessageBody(),
                                        System.currentTimeMillis());
      }
    }
  }

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






































package org.mozilla.gecko;

import java.util.ArrayList;
import java.util.Iterator;

import android.util.Log;

import android.app.PendingIntent;
import android.app.Activity;

import android.content.Intent;
import android.content.BroadcastReceiver;
import android.content.Context;

import android.os.Bundle;

import android.telephony.SmsManager;
import android.telephony.SmsMessage;






class PendingIntentUID
{
  static private int sUID = Integer.MIN_VALUE;

  static public int generate() { return sUID++; }
}





class Envelope
{
  protected int     mId;
  protected int     mRemainingParts;
  protected boolean mFailing;

  public Envelope(int aId, int aParts) {
    mId = aId;
    mRemainingParts = aParts;
    mFailing = false;
  }

  public void decreaseRemainingParts() {
    --mRemainingParts;
  }

  public boolean arePartsRemaining() {
    return mRemainingParts != 0;
  }

  public void markAsFailed() {
    mFailing = true;
  }

  public boolean isFailing() {
    return mFailing;
  }
}




class Postman
{
  public static final int kUnknownEnvelopeId = -1;

  private static final Postman sInstance = new Postman();

  private ArrayList<Envelope> mEnvelopes = new ArrayList<Envelope>(1);

  private Postman() {}

  public static Postman getInstance() {
    return sInstance;
  }

  public int createEnvelope(int aParts) {
    



    int size = mEnvelopes.size();

    for (int i=0; i<size; ++i) {
      if (mEnvelopes.get(i) == null) {
        mEnvelopes.set(i, new Envelope(i, aParts));
        return i;
      }
    }

    mEnvelopes.add(new Envelope(size, aParts));
    return size;
  }

  public Envelope getEnvelope(int aId) {
    if (aId < 0 || mEnvelopes.size() <= aId) {
      Log.e("GeckoSmsManager", "Trying to get an unknown Envelope!");
      return null;
    }

    Envelope envelope = mEnvelopes.get(aId);
    if (envelope == null) {
      Log.e("GeckoSmsManager", "Trying to get an empty Envelope!");
    }

    return envelope;
  }

  public void destroyEnvelope(int aId) {
    if (aId < 0 || mEnvelopes.size() <= aId) {
      Log.e("GeckoSmsManager", "Trying to destroy an unknown Envelope!");
      return;
    }

    if (mEnvelopes.set(aId, null) == null) {
      Log.e("GeckoSmsManager", "Trying to destroy an empty Envelope!");
    }
  }
}

public class GeckoSmsManager
  extends BroadcastReceiver
{
  public final static String ACTION_SMS_RECEIVED = "android.provider.Telephony.SMS_RECEIVED";
  public final static String ACTION_SMS_SENT     = "org.mozilla.gecko.SMS_SENT";
  private final static int kMaxMessageSize = 160;

  @Override
  public void onReceive(Context context, Intent intent) {
    if (intent.getAction().equals(ACTION_SMS_RECEIVED)) {
      
      
      
      
      

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

      return;
    }

    if (intent.getAction().equals(ACTION_SMS_SENT)) {
      Bundle bundle = intent.getExtras();

      if (bundle == null || !bundle.containsKey("envelopeId") ||
          !bundle.containsKey("number") || !bundle.containsKey("message")) {
        Log.e("GeckoSmsManager", "Got an invalid ACTION_SMS_SENT!");
        return;
      }

      int envelopeId = bundle.getInt("envelopeId");
      Postman postman = Postman.getInstance();
      Envelope envelope = postman.getEnvelope(envelopeId);

      envelope.decreaseRemainingParts();

      if (getResultCode() != Activity.RESULT_OK) {
        
        Log.i("GeckoSmsManager", "SMS part sending failed!");
        envelope.markAsFailed();
      }

      if (envelope.arePartsRemaining()) {
        return;
      }

      if (envelope.isFailing()) {
        
        Log.i("GeckoSmsManager", "SMS sending failed!");
      } else {
        
        Log.i("GeckoSmsManager", "SMS sending was successfull!");
      }

      postman.destroyEnvelope(envelopeId);

      return;
    }
  }

  public static int getNumberOfMessagesForText(String aText) {
    return SmsManager.getDefault().divideMessage(aText).size();
  }

  public static void send(String aNumber, String aMessage) {
    




    int envelopeId = Postman.kUnknownEnvelopeId;

    try {
      SmsManager sm = SmsManager.getDefault();

      Intent sentIntent = new Intent(ACTION_SMS_SENT);
      Bundle bundle = new Bundle();
      bundle.putString("number", aNumber);
      bundle.putString("message", aMessage);

      if (aMessage.length() <= kMaxMessageSize) {
        envelopeId = Postman.getInstance().createEnvelope(1);

        bundle.putInt("envelopeId", envelopeId);
        sentIntent.putExtras(bundle);

        










        PendingIntent sentPendingIntent =
          PendingIntent.getBroadcast(GeckoApp.surfaceView.getContext(),
                                     PendingIntentUID.generate(), sentIntent,
                                     PendingIntent.FLAG_CANCEL_CURRENT);

        sm.sendTextMessage(aNumber, "", aMessage, sentPendingIntent, null);
      } else {
        ArrayList<String> parts = sm.divideMessage(aMessage);
        envelopeId = Postman.getInstance().createEnvelope(parts.size());

        bundle.putInt("envelopeId", envelopeId);
        sentIntent.putExtras(bundle);

        ArrayList<PendingIntent> sentPendingIntents =
          new ArrayList<PendingIntent>(parts.size());

        for (int i=0; i<parts.size(); ++i) {
          sentPendingIntents.add(
            PendingIntent.getBroadcast(GeckoApp.surfaceView.getContext(),
                                       PendingIntentUID.generate(), sentIntent,
                                       PendingIntent.FLAG_CANCEL_CURRENT)
          );
        }

        sm.sendMultipartTextMessage(aNumber, "", parts, sentPendingIntents, null);
      }
    } catch (Exception e) {
      Log.e("GeckoSmsManager", "Failed to send an SMS: ", e);

      if (envelopeId != Postman.kUnknownEnvelopeId) {
        Postman.getInstance().destroyEnvelope(envelopeId);
      }
    }
  }
}

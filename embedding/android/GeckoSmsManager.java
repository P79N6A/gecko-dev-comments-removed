




































package org.mozilla.gecko;

import java.util.ArrayList;
import java.util.Iterator;

import android.util.Log;

import android.app.PendingIntent;
import android.app.Activity;

import android.database.Cursor;

import android.content.Intent;
import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.ContentResolver;
import android.content.ContentValues;
import android.content.ContentUris;

import android.net.Uri;

import android.os.Bundle;
import android.os.Handler;
import android.os.Looper;

import android.telephony.SmsManager;
import android.telephony.SmsMessage;






class PendingIntentUID
{
  static private int sUID = Integer.MIN_VALUE;

  static public int generate() { return sUID++; }
}





class Envelope
{
  enum SubParts {
    SENT_PART,
    DELIVERED_PART
  }

  protected int       mId;
  protected int       mMessageId;
  protected long      mMessageTimestamp;

  



  protected int[]     mRemainingParts;

  



  protected boolean[] mFailing;

  


  protected int       mError;

  public Envelope(int aId, int aParts) {
    mId = aId;
    mMessageId = -1;
    mMessageTimestamp = 0;
    mError = GeckoSmsManager.kNoError;

    int size = Envelope.SubParts.values().length;
    mRemainingParts = new int[size];
    mFailing = new boolean[size];

    for (int i=0; i<size; ++i) {
      mRemainingParts[i] = aParts;
      mFailing[i] = false;
    }
  }

  public void decreaseRemainingParts(Envelope.SubParts aType) {
    --mRemainingParts[aType.ordinal()];

    if (mRemainingParts[SubParts.SENT_PART.ordinal()] >
        mRemainingParts[SubParts.DELIVERED_PART.ordinal()]) {
      Log.e("GeckoSmsManager", "Delivered more parts than we sent!?");
    }
  }

  public boolean arePartsRemaining(Envelope.SubParts aType) {
    return mRemainingParts[aType.ordinal()] != 0;
  }

  public void markAsFailed(Envelope.SubParts aType) {
    mFailing[aType.ordinal()] = true;
  }

  public boolean isFailing(Envelope.SubParts aType) {
    return mFailing[aType.ordinal()];
  }

  public int getMessageId() {
    return mMessageId;
  }

  public void setMessageId(int aMessageId) {
    mMessageId = aMessageId;
  }

  public long getMessageTimestamp() {
    return mMessageTimestamp;
  }

  public void setMessageTimestamp(long aMessageTimestamp) {
    mMessageTimestamp = aMessageTimestamp;
  }

  public int getError() {
    return mError;
  }

  public void setError(int aError) {
    mError = aError;
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

class SmsIOThread extends Thread {
  private final static SmsIOThread sInstance = new SmsIOThread();

  private Handler mHandler;

  public static SmsIOThread getInstance() {
    return sInstance;
  }

  public boolean execute(Runnable r) {
    return mHandler.post(r);
  }

  public void run() {
    Looper.prepare();

    mHandler = new Handler();

    Looper.loop();
  }
}

public class GeckoSmsManager
  extends BroadcastReceiver
{
  public final static String ACTION_SMS_RECEIVED  = "android.provider.Telephony.SMS_RECEIVED";
  public final static String ACTION_SMS_SENT      = "org.mozilla.gecko.SMS_SENT";
  public final static String ACTION_SMS_DELIVERED = "org.mozilla.gecko.SMS_DELIVERED";

  



  public final static int kNoError       = 0;
  public final static int kNoSignalError = 1;
  public final static int kUnknownError  = 2;
  public final static int kInternalError = 3;

  private final static int kMaxMessageSize    = 160;

  private final static Uri kSmsContentUri     = Uri.parse("content://sms");
  private final static Uri kSmsSentContentUri = Uri.parse("content://sms/sent");

  private final static int kSmsTypeInbox      = 1;
  private final static int kSmsTypeSentbox    = 2;

  public static void init() {
    SmsIOThread.getInstance().start();
  }

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

    if (intent.getAction().equals(ACTION_SMS_SENT) ||
        intent.getAction().equals(ACTION_SMS_DELIVERED)) {
      Bundle bundle = intent.getExtras();

      if (bundle == null || !bundle.containsKey("envelopeId") ||
          !bundle.containsKey("number") || !bundle.containsKey("message") ||
          !bundle.containsKey("requestId") || !bundle.containsKey("processId")) {
        Log.e("GeckoSmsManager", "Got an invalid ACTION_SMS_SENT/ACTION_SMS_DELIVERED!");
        return;
      }

      int envelopeId = bundle.getInt("envelopeId");
      Postman postman = Postman.getInstance();

      Envelope envelope = postman.getEnvelope(envelopeId);
      if (envelope == null) {
        Log.e("GeckoSmsManager", "Got an invalid envelope id (or Envelope has been destroyed)!");
        return;
      }

      Envelope.SubParts part = intent.getAction().equals(ACTION_SMS_SENT)
                                 ? Envelope.SubParts.SENT_PART
                                 : Envelope.SubParts.DELIVERED_PART;
      envelope.decreaseRemainingParts(part);
 

      if (getResultCode() != Activity.RESULT_OK) {
        switch (getResultCode()) {
          case SmsManager.RESULT_ERROR_NULL_PDU:
            envelope.setError(kInternalError);
            break;
          case SmsManager.RESULT_ERROR_NO_SERVICE:
          case SmsManager.RESULT_ERROR_RADIO_OFF:
            envelope.setError(kNoSignalError);
            break;
          case SmsManager.RESULT_ERROR_GENERIC_FAILURE:
          default:
            envelope.setError(kUnknownError);
            break;
        }
        envelope.markAsFailed(part);
        Log.i("GeckoSmsManager", "SMS part sending failed!");
      }

      if (envelope.arePartsRemaining(part)) {
        return;
      }

      if (envelope.isFailing(part)) {
        if (part == Envelope.SubParts.SENT_PART) {
          GeckoAppShell.notifySmsSendFailed(envelope.getError(),
                                            bundle.getInt("requestId"),
                                            bundle.getLong("processId"));
          Log.i("GeckoSmsManager", "SMS sending failed!");
        } else {
          
          
          Log.e("GeckoSmsManager", "SMS failed to be delivered... is that even possible?");
        }
      } else {
        if (part == Envelope.SubParts.SENT_PART) {
          String number = bundle.getString("number");
          String message = bundle.getString("message");
          long timestamp = System.currentTimeMillis();

          int id = GeckoAppShell.saveMessageInSentbox(number, message, timestamp);

          GeckoAppShell.notifySmsSent(id, number, message, timestamp,
                                      bundle.getInt("requestId"),
                                      bundle.getLong("processId"));

          envelope.setMessageId(id);
          envelope.setMessageTimestamp(timestamp);

          Log.i("GeckoSmsManager", "SMS sending was successfull!");
        } else {
          GeckoAppShell.notifySmsDelivered(envelope.getMessageId(),
                                           bundle.getString("number"),
                                           bundle.getString("message"),
                                           envelope.getMessageTimestamp());
          Log.i("GeckoSmsManager", "SMS succesfully delivered!");
        }
      }

      
      if (!envelope.arePartsRemaining(Envelope.SubParts.SENT_PART) &&
          !envelope.arePartsRemaining(Envelope.SubParts.DELIVERED_PART)) {
        postman.destroyEnvelope(envelopeId);
      }

      return;
    }
  }

  public static int getNumberOfMessagesForText(String aText) {
    return SmsManager.getDefault().divideMessage(aText).size();
  }

  public static void send(String aNumber, String aMessage, int aRequestId, long aProcessId) {
    int envelopeId = Postman.kUnknownEnvelopeId;

    try {
      SmsManager sm = SmsManager.getDefault();

      Intent sentIntent = new Intent(ACTION_SMS_SENT);
      Intent deliveredIntent = new Intent(ACTION_SMS_DELIVERED);

      Bundle bundle = new Bundle();
      bundle.putString("number", aNumber);
      bundle.putString("message", aMessage);
      bundle.putInt("requestId", aRequestId);
      bundle.putLong("processId", aProcessId);

      if (aMessage.length() <= kMaxMessageSize) {
        envelopeId = Postman.getInstance().createEnvelope(1);
        bundle.putInt("envelopeId", envelopeId);

        sentIntent.putExtras(bundle);
        deliveredIntent.putExtras(bundle);

        










        PendingIntent sentPendingIntent =
          PendingIntent.getBroadcast(GeckoApp.surfaceView.getContext(),
                                     PendingIntentUID.generate(), sentIntent,
                                     PendingIntent.FLAG_CANCEL_CURRENT);

        PendingIntent deliveredPendingIntent =
          PendingIntent.getBroadcast(GeckoApp.surfaceView.getContext(),
                                     PendingIntentUID.generate(), deliveredIntent,
                                     PendingIntent.FLAG_CANCEL_CURRENT);

        sm.sendTextMessage(aNumber, "", aMessage,
                           sentPendingIntent, deliveredPendingIntent);
      } else {
        ArrayList<String> parts = sm.divideMessage(aMessage);
        envelopeId = Postman.getInstance().createEnvelope(parts.size());
        bundle.putInt("envelopeId", envelopeId);

        sentIntent.putExtras(bundle);
        deliveredIntent.putExtras(bundle);

        ArrayList<PendingIntent> sentPendingIntents =
          new ArrayList<PendingIntent>(parts.size());
        ArrayList<PendingIntent> deliveredPendingIntents =
          new ArrayList<PendingIntent>(parts.size());

        for (int i=0; i<parts.size(); ++i) {
          sentPendingIntents.add(
            PendingIntent.getBroadcast(GeckoApp.surfaceView.getContext(),
                                       PendingIntentUID.generate(), sentIntent,
                                       PendingIntent.FLAG_CANCEL_CURRENT)
          );

          deliveredPendingIntents.add(
            PendingIntent.getBroadcast(GeckoApp.surfaceView.getContext(),
                                       PendingIntentUID.generate(), deliveredIntent,
                                       PendingIntent.FLAG_CANCEL_CURRENT)
          );
        }

        sm.sendMultipartTextMessage(aNumber, "", parts, sentPendingIntents,
                                    deliveredPendingIntents);
      }
    } catch (Exception e) {
      Log.e("GeckoSmsManager", "Failed to send an SMS: ", e);

      if (envelopeId != Postman.kUnknownEnvelopeId) {
        Postman.getInstance().destroyEnvelope(envelopeId);
      }

      GeckoAppShell.notifySmsSendFailed(kUnknownError, aRequestId, aProcessId);
    }
  }

  public static int saveSentMessage(String aRecipient, String aBody, long aDate) {
    class IdTooHighException extends Exception { }

    try {
      ContentValues values = new ContentValues();
      values.put("address", aRecipient);
      values.put("body", aBody);
      values.put("date", aDate);

      ContentResolver cr = GeckoApp.surfaceView.getContext().getContentResolver();
      Uri uri = cr.insert(kSmsSentContentUri, values);

      long id = ContentUris.parseId(uri);

      
      
      if (id > Integer.MAX_VALUE) {
        throw new IdTooHighException();
      }

      return (int)id;
    } catch (IdTooHighException e) {
      Log.e("GeckoSmsManager", "The id we received is higher than the higher allowed value.");
      return -1;
    } catch (Exception e) {
      Log.e("GeckoSmsManager", "Something went wrong when trying to write a sent message: " + e);
      return -1;
    }
  }

  public static void getMessage(int aMessageId, int aRequestId, long aProcessId) {
    class GetMessageRunnable implements Runnable {
      private int mMessageId;
      private int mRequestId;
      private long mProcessId;

      GetMessageRunnable(int aMessageId, int aRequestId, long aProcessId) {
        mMessageId = aMessageId;
        mRequestId = aRequestId;
        mProcessId = aProcessId;
      }

      @Override
      public void run() {
        class NotFoundException extends Exception { }
        class UnmatchingIdException extends Exception { }
        class TooManyResultsException extends Exception { }
        class InvalidTypeException extends Exception { }

        Cursor cursor = null;

        try {
          ContentResolver cr = GeckoApp.surfaceView.getContext().getContentResolver();
          Uri message = ContentUris.withAppendedId(kSmsContentUri, mMessageId);

          cursor = cr.query(message,
                            new String[] { "_id", "address", "body", "date", "type" },
                            null, null, null);
          if (cursor == null || cursor.getCount() == 0) {
            throw new NotFoundException();
          }

          if (cursor.getCount() != 1) {
            throw new TooManyResultsException();
          }

          cursor.moveToFirst();

          if (cursor.getInt(cursor.getColumnIndex("_id")) != mMessageId) {
            throw new UnmatchingIdException();
          }

          int type = cursor.getInt(cursor.getColumnIndex("type"));
          String sender = "";
          String receiver = "";

          if (type == kSmsTypeInbox) {
            sender = cursor.getString(cursor.getColumnIndex("address"));
          } else if (type == kSmsTypeSentbox) {
            receiver = cursor.getString(cursor.getColumnIndex("address"));
          } else {
            throw new InvalidTypeException();
          }

          GeckoAppShell.notifyGetSms(cursor.getInt(cursor.getColumnIndex("_id")),
                                     receiver, sender,
                                     cursor.getString(cursor.getColumnIndex("body")),
                                     cursor.getLong(cursor.getColumnIndex("date")),
                                     mRequestId, mProcessId);
        } catch (NotFoundException e) {
          
          Log.i("GeckoSmsManager", "Message id " + mMessageId + " not found");
        } catch (UnmatchingIdException e) {
          
          Log.e("GeckoSmsManager", "Requested message id (" + mMessageId +
                                   ") is different from the one we got.");
        } catch (TooManyResultsException e) {
          
          Log.e("GeckoSmsManager", "Get too many results for id " + mMessageId);
        } catch (InvalidTypeException e) {
          
          Log.i("GeckoSmsManager", "Message has an invalid type, we ignore it.");
        } catch (Exception e) {
          
          Log.e("GeckoSmsManager", "Error while trying to get message: " + e);
        } finally {
          if (cursor != null) {
            cursor.close();
          }
        }
      }
    }

    if (!SmsIOThread.getInstance().execute(new GetMessageRunnable(aMessageId, aRequestId, aProcessId))) {
      
      Log.e("GeckoSmsManager", "Failed to add GetMessageRunnable to the SmsIOThread");
    }
  }

  public static void shutdown() {
    SmsIOThread.getInstance().interrupt();
  }
}

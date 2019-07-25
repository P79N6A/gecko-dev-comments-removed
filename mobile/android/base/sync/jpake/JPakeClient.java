




































package org.mozilla.gecko.sync.jpake;

import java.io.IOException;
import java.io.Reader;
import java.io.StringReader;
import java.io.UnsupportedEncodingException;
import java.math.BigInteger;
import java.net.URISyntaxException;
import java.security.GeneralSecurityException;
import java.util.Random;
import java.util.Timer;
import java.util.TimerTask;

import org.json.simple.JSONObject;
import org.json.simple.parser.JSONParser;
import org.json.simple.parser.ParseException;
import org.mozilla.apache.commons.codec.binary.Base64;
import org.mozilla.gecko.sync.ExtendedJSONObject;
import org.mozilla.gecko.sync.Logger;
import org.mozilla.gecko.sync.NonObjectJSONException;
import org.mozilla.gecko.sync.ThreadPool;
import org.mozilla.gecko.sync.Utils;
import org.mozilla.gecko.sync.crypto.CryptoException;
import org.mozilla.gecko.sync.crypto.CryptoInfo;
import org.mozilla.gecko.sync.crypto.Cryptographer;
import org.mozilla.gecko.sync.crypto.KeyBundle;
import org.mozilla.gecko.sync.crypto.NoKeyBundleException;
import org.mozilla.gecko.sync.net.ResourceDelegate;
import org.mozilla.gecko.sync.net.SyncResourceDelegate;
import org.mozilla.gecko.sync.setup.Constants;
import org.mozilla.gecko.sync.setup.activities.SetupSyncActivity;

import android.util.Log;
import ch.boye.httpclientandroidlib.Header;
import ch.boye.httpclientandroidlib.HttpResponse;
import ch.boye.httpclientandroidlib.client.ClientProtocolException;
import ch.boye.httpclientandroidlib.client.methods.HttpRequestBase;
import ch.boye.httpclientandroidlib.entity.StringEntity;
import ch.boye.httpclientandroidlib.impl.client.DefaultHttpClient;
import ch.boye.httpclientandroidlib.message.BasicHeader;
import java.security.NoSuchAlgorithmException;
import java.security.InvalidKeyException;

public class JPakeClient implements JPakeRequestDelegate {
  private static String       LOG_TAG                 = "JPakeClient";

  
  private final static int    REQUEST_TIMEOUT         = 60 * 1000;         
                                                                            
  private final static int    SOCKET_TIMEOUT          = 5 * 60 * 1000;     
                                                                            
  private final static int    KEYEXCHANGE_VERSION     = 3;

  private final static String JPAKE_SIGNERID_SENDER   = "sender";
  private final static String JPAKE_SIGNERID_RECEIVER = "receiver";
  private final static int    JPAKE_LENGTH_SECRET     = 8;
  private final static int    JPAKE_LENGTH_CLIENTID   = 256;
  private final static String JPAKE_VERIFY_VALUE      = "0123456789ABCDEF";

  private final static int    MAX_TRIES               = 10;
  private final static int    MAX_TRIES_FIRST_MSG     = 300;
  private final static int    MAX_TRIES_LAST_MSG      = 300;

  
  private SetupSyncActivity   ssActivity;

  
  private String              clientId;
  private String              secret;

  private String              myEtag;
  private String              mySignerId;
  private String              theirEtag;
  private String              theirSignerId;
  private String              jpakeServer;

  
  private boolean             pairWithPin;
  private boolean             paired                  = false;
  private boolean             finished                = false;
  private State               state                   = State.GET_CHANNEL;
  private State               stateContext;
  private String              error;
  private int                 pollTries               = 0;

  
  private int                 jpakePollInterval;
  private int                 jpakeMaxTries;
  private String              channel;
  private String              channelUrl;

  
  private Timer               timerScheduler;

  
  private KeyBundle           myKeyBundle;

  private ExtendedJSONObject  jOutgoing;
  private ExtendedJSONObject  jIncoming;

  private JPakeParty          jParty;
  private JPakeNumGenerator   numGen;

  public JPakeClient(SetupSyncActivity activity) {
    ssActivity = activity;

    
    
    jpakeServer = "https://setup.services.mozilla.com/";
    jpakePollInterval = 1 * 1000; 
    jpakeMaxTries = MAX_TRIES;

    if (!jpakeServer.endsWith("/")) {
      jpakeServer += "/";
    }

    timerScheduler = new Timer();

    setClientId();
  }

  







  public void receiveNoPin() {
    mySignerId = JPAKE_SIGNERID_RECEIVER;
    theirSignerId = JPAKE_SIGNERID_SENDER;
    pairWithPin = false;

    
    jpakeMaxTries = MAX_TRIES_FIRST_MSG;

    jParty = new JPakeParty(mySignerId);
    numGen = new JPakeNumGeneratorRandom();

    final JPakeClient self = this;
    runOnThread(new Runnable() {
      @Override
      public void run() {
        self.createSecret();
        self.getChannel();
      }
    });
  }

  













  public void pairWithPin(String pin, boolean expectDelay) {
    mySignerId = JPAKE_SIGNERID_SENDER;
    theirSignerId = JPAKE_SIGNERID_RECEIVER;
    pairWithPin = true;

    
    secret = pin.substring(0, JPAKE_LENGTH_SECRET);
    channel = pin.substring(JPAKE_LENGTH_SECRET);
    channelUrl = jpakeServer + channel;

    jParty = new JPakeParty(mySignerId);
    numGen = new JPakeNumGeneratorRandom();

    final JPakeClient self = this;
    runOnThread(new Runnable() {
      @Override
      public void run() {
        
        self.state = State.SNDR_STEP_ZERO;
        scheduleGetRequest(jpakePollInterval);
      }
    });
  }

  private static void runOnThread(Runnable run) {
    ThreadPool.run(run);
  }

  public enum State {
    GET_CHANNEL, SNDR_STEP_ZERO, SNDR_STEP_ONE, SNDR_STEP_TWO, RCVR_STEP_ONE, RCVR_STEP_TWO, PUT, ABORT, ENCRYPT_PUT, REPORT_FAILURE, VERIFY_KEY, VERIFY_PAIRING;
  }

  

  


  private void getChannel() {
    Logger.debug(LOG_TAG, "Getting channel.");
    if (finished) {
      Logger.debug(LOG_TAG, "Finished; returning.");
      return;
    }

    try {
      final String uri = jpakeServer + "new_channel";
      Logger.debug(LOG_TAG, "Fetching " + uri);
      JPakeRequest channelRequest = new JPakeRequest(uri, makeRequestResourceDelegate());
      channelRequest.get();
    } catch (URISyntaxException e) {
      Log.e(LOG_TAG, "URISyntaxException", e);
      abort(Constants.JPAKE_ERROR_CHANNEL);
      return;
    } catch (Exception e) {
      Log.e(LOG_TAG, "Unexpected exception in getChannel().", e);
      abort(Constants.JPAKE_ERROR_CHANNEL);
      return;
    }
  }

  



  private void putStep() {
    Logger.debug(LOG_TAG, "Uploading message.");
    runOnThread(new Runnable() {
      @Override
      public void run() {
        JPakeRequest putRequest = null;
        try {
          putRequest = new JPakeRequest(channelUrl,
              makeRequestResourceDelegate());
        } catch (URISyntaxException e) {
          Log.e(LOG_TAG, "URISyntaxException", e);
          abort(Constants.JPAKE_ERROR_CHANNEL);
          return;
        }
        try {
          putRequest.put(jsonEntity(jOutgoing.object));
        } catch (UnsupportedEncodingException e) {
          e.printStackTrace();
        }
        Logger.debug(LOG_TAG, "outgoing: " + jOutgoing.toJSONString());
      }
    });
  }

  


  private void computeStepOne() throws NoSuchAlgorithmException, UnsupportedEncodingException {
    Logger.debug(LOG_TAG, "Computing round 1.");

    JPakeCrypto.round1(jParty, numGen);

    
    ExtendedJSONObject jOne = new ExtendedJSONObject();
    jOne.put(Constants.ZKP_KEY_GX1,
        BigIntegerHelper.toEvenLengthHex(jParty.gx1));
    jOne.put(Constants.ZKP_KEY_GX2,
        BigIntegerHelper.toEvenLengthHex(jParty.gx2));

    Zkp zkp1 = jParty.zkp1;
    Zkp zkp2 = jParty.zkp2;
    ExtendedJSONObject jZkp1 = makeJZkp(zkp1.gr, zkp1.b, mySignerId);
    ExtendedJSONObject jZkp2 = makeJZkp(zkp2.gr, zkp2.b, mySignerId);

    jOne.put(Constants.ZKP_KEY_ZKP_X1, jZkp1);
    jOne.put(Constants.ZKP_KEY_ZKP_X2, jZkp2);

    jOutgoing = new ExtendedJSONObject();
    jOutgoing.put(Constants.JSON_KEY_TYPE, mySignerId + "1");
    jOutgoing.put(Constants.JSON_KEY_PAYLOAD, jOne);
    jOutgoing.put(Constants.JSON_KEY_VERSION, KEYEXCHANGE_VERSION);
    Logger.debug(LOG_TAG, "Sending: " + jOutgoing.toJSONString());

    
    stateContext = pairWithPin ? State.SNDR_STEP_ONE : State.RCVR_STEP_ONE;
    state = State.PUT;
    putStep();
  }

  





  private void computeStepTwo() throws NonObjectJSONException {
    Logger.debug(LOG_TAG, "Computing round 2.");

    
    if (!jIncoming.get(Constants.JSON_KEY_TYPE).equals(theirSignerId + "1")) {
      Log.e(LOG_TAG, "Invalid round 1 message: " + jIncoming.toJSONString());
      abort(Constants.JPAKE_ERROR_WRONGMESSAGE);
      return;
    }

    
    ExtendedJSONObject iPayload = jIncoming.getObject(Constants.JSON_KEY_PAYLOAD);
    if (iPayload == null) {
      Log.e(LOG_TAG, "Invalid round 1 message: " + jIncoming.toJSONString());
      abort(Constants.JPAKE_ERROR_WRONGMESSAGE);
      return;
    }

    ExtendedJSONObject zkpPayload3 = iPayload.getObject(Constants.ZKP_KEY_ZKP_X1);
    ExtendedJSONObject zkpPayload4 = iPayload.getObject(Constants.ZKP_KEY_ZKP_X2);
    if (zkpPayload3 == null || zkpPayload4 == null) {
      Log.e(LOG_TAG, "Invalid round 1 zkpPayload message");
      abort(Constants.JPAKE_ERROR_WRONGMESSAGE);
      return;
    }

    if (!theirSignerId.equals(zkpPayload3.get(Constants.ZKP_KEY_ID)) ||
        !theirSignerId.equals(zkpPayload4.get(Constants.ZKP_KEY_ID))) {
      Log.e(LOG_TAG, "Invalid round 1 zkpPayload message");
      abort(Constants.JPAKE_ERROR_WRONGMESSAGE);
      return;
    }

    
    jParty.gx3 = new BigInteger((String) iPayload.get(Constants.ZKP_KEY_GX1), 16);
    jParty.gx4 = new BigInteger((String) iPayload.get(Constants.ZKP_KEY_GX2), 16);

    
    String zkp3_gr = (String) zkpPayload3.get(Constants.ZKP_KEY_GR);
    String zkp3_b  = (String) zkpPayload3.get(Constants.ZKP_KEY_B);
    String zkp3_id = (String) zkpPayload3.get(Constants.ZKP_KEY_ID);

    String zkp4_gr = (String) zkpPayload4.get(Constants.ZKP_KEY_GR);
    String zkp4_b  = (String) zkpPayload4.get(Constants.ZKP_KEY_B);
    String zkp4_id = (String) zkpPayload4.get(Constants.ZKP_KEY_ID);

    jParty.zkp3 = new Zkp(new BigInteger(zkp3_gr, 16), new BigInteger(zkp3_b, 16), zkp3_id);
    jParty.zkp4 = new Zkp(new BigInteger(zkp4_gr, 16), new BigInteger(zkp4_b, 16), zkp4_id);

    
    try {
      JPakeCrypto.round2(JPakeClient.secretAsBigInteger(secret), jParty, numGen);
    } catch (Gx3OrGx4IsZeroOrOneException e) {
      Log.e(LOG_TAG, "gx3 and gx4 cannot equal 0 or 1.");
      abort(Constants.JPAKE_ERROR_INTERNAL);
      return;
    } catch (IncorrectZkpException e) {
      Log.e(LOG_TAG, "ZKP mismatch");
      abort(Constants.JPAKE_ERROR_WRONGMESSAGE);
      return;
    } catch (NoSuchAlgorithmException e) {
      Log.e(LOG_TAG, "NoSuchAlgorithmException", e);
      abort(Constants.JPAKE_ERROR_INTERNAL);
      return;
    } catch (UnsupportedEncodingException e) {
      Log.e(LOG_TAG, "UnsupportedEncodingException", e);
      abort(Constants.JPAKE_ERROR_INTERNAL);
      return;
    }

    
    Zkp zkpA = jParty.thisZkpA;
    ExtendedJSONObject oPayload = new ExtendedJSONObject();
    ExtendedJSONObject jZkpA = makeJZkp(zkpA.gr, zkpA.b, zkpA.id);
    oPayload.put(Constants.ZKP_KEY_A, BigIntegerHelper.toEvenLengthHex(jParty.thisA));
    oPayload.put(Constants.ZKP_KEY_ZKP_A, jZkpA);

    
    jOutgoing = new ExtendedJSONObject();
    jOutgoing.put(Constants.JSON_KEY_TYPE, mySignerId + "2");
    jOutgoing.put(Constants.JSON_KEY_VERSION, KEYEXCHANGE_VERSION);
    jOutgoing.put(Constants.JSON_KEY_PAYLOAD, oPayload);

    
    if (pairWithPin) {
      state = State.SNDR_STEP_TWO;
      stateContext = State.PUT;
      scheduleGetRequest(jpakePollInterval);
    } else {
      stateContext = State.RCVR_STEP_TWO;
      state = State.PUT;
      putStep();
    }
  }

  





  private void computeFinal() throws NonObjectJSONException {
    Logger.debug(LOG_TAG, "Computing final round.");
    
    if (!jIncoming.get(Constants.JSON_KEY_TYPE).equals(theirSignerId + "2")) {
      Log.e(LOG_TAG, "Invalid round 2 message: " + jIncoming.toJSONString());
      abort(Constants.JPAKE_ERROR_WRONGMESSAGE);
      return;
    }

    
    ExtendedJSONObject iPayload = jIncoming.getObject(Constants.JSON_KEY_PAYLOAD);
    if (iPayload == null ||
        iPayload.getObject(Constants.ZKP_KEY_ZKP_A) == null) {
      Log.e(LOG_TAG, "Invalid round 2 message: " + jIncoming.toJSONString());
      abort(Constants.JPAKE_ERROR_WRONGMESSAGE);
      return;
    }
    ExtendedJSONObject zkpPayload = iPayload.getObject(Constants.ZKP_KEY_ZKP_A);
    if (!theirSignerId.equals(zkpPayload.get(Constants.ZKP_KEY_ID))) {
      Log.e(LOG_TAG, "Invalid round 2 message: " + jIncoming.toJSONString());
      abort(Constants.JPAKE_ERROR_WRONGMESSAGE);
      return;
    }

    
    jParty.otherA = new BigInteger((String) iPayload.get(Constants.ZKP_KEY_A), 16);

    
    String gr = (String) zkpPayload.get(Constants.ZKP_KEY_GR);
    String b  = (String) zkpPayload.get(Constants.ZKP_KEY_B);
    String id = (String) zkpPayload.get(Constants.ZKP_KEY_ID);

    jParty.otherZkpA = new Zkp(new BigInteger(gr, 16), new BigInteger(b, 16), id);

    myKeyBundle = null;
    try {
      myKeyBundle = JPakeCrypto.finalRound(JPakeClient.secretAsBigInteger(secret), jParty);
    } catch (IncorrectZkpException e) {
      Log.e(LOG_TAG, "ZKP mismatch");
      abort(Constants.JPAKE_ERROR_WRONGMESSAGE);
      return;
    } catch (NoSuchAlgorithmException e) {
      Log.e(LOG_TAG, "NoSuchAlgorithmException", e);
      abort(Constants.JPAKE_ERROR_INTERNAL);
      return;
    } catch (InvalidKeyException e) {
      Log.e(LOG_TAG, "InvalidKeyException", e);
      abort(Constants.JPAKE_ERROR_INTERNAL);
      return;
    } catch (UnsupportedEncodingException e) {
      Log.e(LOG_TAG, "UnsupportedEncodingException", e);
      abort(Constants.JPAKE_ERROR_INTERNAL);
      return;
    }

    if (pairWithPin) { 
      Logger.debug(LOG_TAG, "get: verifyPairing");
      this.state = State.VERIFY_PAIRING;
      scheduleGetRequest(jpakePollInterval);
    } else { 
      try {
        jOutgoing = computeKeyVerification(myKeyBundle);
      } catch (UnsupportedEncodingException e) {
        Log.e(LOG_TAG, "Failed to encrypt key verification value.", e);
        abort(Constants.JPAKE_ERROR_INTERNAL);
        return;
      } catch (CryptoException e) {
        Log.e(LOG_TAG, "Failed to encrypt key verification value.", e);
        abort(Constants.JPAKE_ERROR_INTERNAL);
        return;
      }

      stateContext = State.VERIFY_KEY;
      this.state = State.PUT;
      putStep();
    }
  }

  



  public ExtendedJSONObject computeKeyVerification(KeyBundle keyBundle)
      throws UnsupportedEncodingException, CryptoException
  {
    Logger.debug(LOG_TAG, "Encrypting key verification value.");
    
    ExtendedJSONObject jPayload = encryptPayload(JPAKE_VERIFY_VALUE, keyBundle);
    ExtendedJSONObject result = new ExtendedJSONObject();
    result.put(Constants.JSON_KEY_TYPE, mySignerId + "3");
    result.put(Constants.JSON_KEY_VERSION, KEYEXCHANGE_VERSION);
    result.put(Constants.JSON_KEY_PAYLOAD, jPayload.object);
    return result;
  }

  



  private boolean verifyPairing(ExtendedJSONObject verificationObject,
      KeyBundle keyBundle) throws CryptoException, IOException, ParseException,
      NonObjectJSONException {
    if (!verificationObject.get(Constants.JSON_KEY_TYPE).equals(
        theirSignerId + "3")) {
      Log.e(LOG_TAG, "Invalid round 3 message: " + verificationObject.toJSONString());
      abort(Constants.JPAKE_ERROR_WRONGMESSAGE);
      return false;
    }
    ExtendedJSONObject payload = verificationObject
        .getObject(Constants.JSON_KEY_PAYLOAD);
    String theirCiphertext = (String) payload
        .get(Constants.JSON_KEY_CIPHERTEXT);
    String iv = (String) payload.get(Constants.JSON_KEY_IV);
    boolean correctPairing = verifyCiphertext(theirCiphertext, iv, keyBundle);
    return correctPairing;
  }

  



  public boolean verifyCiphertext(String theirCiphertext, String iv,
      KeyBundle keyBundle) throws UnsupportedEncodingException, CryptoException {
    byte[] cleartextBytes = JPAKE_VERIFY_VALUE.getBytes("UTF-8");
    CryptoInfo info = new CryptoInfo(cleartextBytes, keyBundle);
    info.setIV(Base64.decodeBase64(iv));

    Cryptographer.encrypt(info);
    String myCiphertext = new String(Base64.encodeBase64(info.getMessage()));
    return myCiphertext.equals(theirCiphertext);
  }

  




  public void sendAndComplete(JSONObject jObj)
      throws JPakeNoActivePairingException {
    if (paired && !finished) {
      String outData = jObj.toJSONString();
      state = State.ENCRYPT_PUT;
      encryptData(myKeyBundle, outData);
      putStep();
    } else {
      Log.e(LOG_TAG, "Can't send data, no active pairing!");
      throw new JPakeNoActivePairingException();
    }
  }

  







  private void encryptData(KeyBundle keyBundle, String payload) {
    Logger.debug(LOG_TAG, "Encrypting data.");
    ExtendedJSONObject jPayload = null;
    try {
      jPayload = encryptPayload(payload, keyBundle);
    } catch (UnsupportedEncodingException e) {
      Log.e(LOG_TAG, "Failed to encrypt data.", e);
      abort(Constants.JPAKE_ERROR_INTERNAL);
      return;
    } catch (CryptoException e) {
      Log.e(LOG_TAG, "Failed to encrypt data.", e);
      abort(Constants.JPAKE_ERROR_INTERNAL);
      return;
    }
    jOutgoing = new ExtendedJSONObject();
    jOutgoing.put(Constants.JSON_KEY_TYPE, mySignerId + "3");
    jOutgoing.put(Constants.JSON_KEY_VERSION, KEYEXCHANGE_VERSION);
    jOutgoing.put(Constants.JSON_KEY_PAYLOAD, jPayload.object);
  }

  






  private void decryptData(KeyBundle keyBundle) {
    Logger.debug(LOG_TAG, "Verifying their key");
    if (!(theirSignerId + "3").equals((String) jIncoming
        .get(Constants.JSON_KEY_TYPE))) {
      try {
        Log.e(LOG_TAG, "Invalid round 3 data: " + jsonEntity(jIncoming.object));
      } catch (UnsupportedEncodingException e) {
        e.printStackTrace();
      }
      abort(Constants.JPAKE_ERROR_WRONGMESSAGE);
      return;
    }

    
    ExtendedJSONObject iPayload = null;
    try {
      iPayload = jIncoming.getObject(Constants.JSON_KEY_PAYLOAD);
    } catch (NonObjectJSONException e1) {
      Log.e(LOG_TAG, "Invalid round 3 data.", e1);
      abort(Constants.JPAKE_ERROR_WRONGMESSAGE);
      return;
    }
    Logger.debug(LOG_TAG, "Decrypting data.");
    String cleartext = null;
    try {
      cleartext = new String(decryptPayload(iPayload, keyBundle), "UTF-8");
    } catch (UnsupportedEncodingException e1) {
      Log.e(LOG_TAG, "Failed to decrypt data.", e1);
      abort(Constants.JPAKE_ERROR_INTERNAL);
      return;
    } catch (CryptoException e1) {
      Log.e(LOG_TAG, "Failed to decrypt data.", e1);
      abort(Constants.JPAKE_ERROR_KEYMISMATCH);
      return;
    }
    JSONObject jCreds = null;
    try {
      jCreds = getJSONObject(cleartext);
    } catch (Exception e) {
      Log.e(LOG_TAG, "Invalid data: " + cleartext);
      abort(Constants.JPAKE_ERROR_INVALID);
      return;
    }
    complete(jCreds);
  }

  




  private void complete(JSONObject jCreds) {
    Logger.debug(LOG_TAG, "Exchange complete.");
    finished = true;
    ssActivity.onComplete(jCreds);
  }

  
  @Override
  public void onRequestFailure(HttpResponse res) {
    JPakeResponse response = new JPakeResponse(res);
    switch (this.state) {
    case GET_CHANNEL:
      Log.e(LOG_TAG, "getChannel failure: " + response.getStatusCode());
      abort(Constants.JPAKE_ERROR_CHANNEL);
      break;
    case VERIFY_PAIRING:
    case VERIFY_KEY:
    case SNDR_STEP_ZERO:
    case SNDR_STEP_ONE:
    case SNDR_STEP_TWO:
    case RCVR_STEP_ONE:
    case RCVR_STEP_TWO:
      int statusCode = response.getStatusCode();
      switch (statusCode) {
      case 304:
        Logger.debug(LOG_TAG, "Channel hasn't been updated yet. Will try again later");
        if (pollTries >= jpakeMaxTries) {
          Log.e(LOG_TAG, "Tried for " + pollTries + " times, maxTries " + jpakeMaxTries + ", aborting");
          abort(Constants.JPAKE_ERROR_TIMEOUT);
          return;
        }
        pollTries += 1;
        if (!finished) {
          scheduleGetRequest(jpakePollInterval);
        }
        return;
      case 404:
        Log.e(LOG_TAG, "No data found in channel.");
        abort(Constants.JPAKE_ERROR_NODATA);
        break;
      case 412: 
        Logger.debug(LOG_TAG, "Message already replaced on server by other party.");
        onRequestSuccess(res);
        break;
      default:
        Log.e(LOG_TAG, "Could not retrieve data. Server responded with HTTP " + statusCode);
        abort(Constants.JPAKE_ERROR_SERVER);
        return;
      }
      pollTries = 0;
      break;
    case PUT:
      Log.e(LOG_TAG, "Could not upload data. Server responded with HTTP " + response.getStatusCode());
      abort(Constants.JPAKE_ERROR_SERVER);
      break;
    case ABORT:
      Log.e(LOG_TAG, "Abort: request failure.");
      break;
    case REPORT_FAILURE:
      Log.e(LOG_TAG, "ReportFailure: failure. Server responded with HTTP "
          + response.getStatusCode());
      break;
    default:
      Log.e(LOG_TAG, "Unhandled request failure " + response.getStatusCode());
    }
  }

  @Override
  public void onRequestError(Exception e) {
    abort(Constants.JPAKE_ERROR_NETWORK);
  }

  @Override
  public void onRequestSuccess(HttpResponse res) {
    if (finished)
      return;
    JPakeResponse response = new JPakeResponse(res);
    Header[] etagHeaders;
    switch (this.state) {
    case GET_CHANNEL:
      Object body = null;
      try {
        body = response.jsonBody();
      } catch (IllegalStateException e1) {
        e1.printStackTrace();
      } catch (IOException e1) {
        e1.printStackTrace();
      } catch (ParseException e1) {
        abort(Constants.JPAKE_ERROR_CHANNEL);
      }
      String channel = body instanceof String ? (String) body : null;
      if (channel == null) { 
        abort(Constants.JPAKE_ERROR_CHANNEL);
        return;
      }
      channelUrl = jpakeServer + channel;
      Logger.debug(LOG_TAG, "using channel " + channel);

      ssActivity.displayPin(secret + channel);

      
      this.state = State.RCVR_STEP_ONE;
      try {
        computeStepOne();
      } catch (NoSuchAlgorithmException e) {
        Log.e(LOG_TAG, "NoSuchAlgorithmException", e);
        abort(Constants.JPAKE_ERROR_INTERNAL);
        return;
      } catch (UnsupportedEncodingException e) {
        Log.e(LOG_TAG, "UnsupportedEncodingException", e);
        abort(Constants.JPAKE_ERROR_INTERNAL);
        return;
      }
      break;

    
    case RCVR_STEP_ONE:
      ssActivity.onPairingStart();
      jpakeMaxTries = MAX_TRIES;
      
    case RCVR_STEP_TWO:
    case SNDR_STEP_ZERO:
    case SNDR_STEP_ONE:
    case SNDR_STEP_TWO:
    case VERIFY_KEY:
    case VERIFY_PAIRING:
      etagHeaders = response.httpResponse().getHeaders("etag");
      if (etagHeaders == null) {
        try {
          Log.e(LOG_TAG, "Server did not supply ETag for message: " + response.body());
          abort(Constants.JPAKE_ERROR_SERVER);
        } catch (IllegalStateException e) {
          e.printStackTrace();
        } catch (IOException e) {
          e.printStackTrace();
        }
        return;
      }

      theirEtag = etagHeaders[0].toString();
      try {
        jIncoming = response.jsonObjectBody();
      } catch (IllegalStateException e) {
        e.printStackTrace();
      } catch (IOException e) {
        e.printStackTrace();
      } catch (ParseException e) {
        abort(Constants.JPAKE_ERROR_INVALID);
        return;
      } catch (NonObjectJSONException e) {
        abort(Constants.JPAKE_ERROR_INVALID);
        return;
      }
      Logger.debug(LOG_TAG, "incoming message: " + jIncoming.toJSONString());

      if (this.state == State.SNDR_STEP_ZERO) {
        try {
          computeStepOne();
        } catch (NoSuchAlgorithmException e) {
          Log.e(LOG_TAG, "NoSuchAlgorithmException", e);
          abort(Constants.JPAKE_ERROR_INTERNAL);
          return;
        } catch (UnsupportedEncodingException e) {
          Log.e(LOG_TAG, "UnsupportedEncodingException", e);
          abort(Constants.JPAKE_ERROR_INTERNAL);
          return;
        }
      } else if (this.state == State.RCVR_STEP_ONE
          || this.state == State.SNDR_STEP_ONE) {
        try {
          computeStepTwo();
        } catch (NonObjectJSONException e) {
          Log.e(LOG_TAG, "NonObjectJSONException", e);
          abort(Constants.JPAKE_ERROR_INVALID);
          return;
        }
      } else if (this.state == State.SNDR_STEP_TWO) {
        stateContext = state;
        state = State.PUT;
        putStep();
      } else if (this.state == State.RCVR_STEP_TWO) {
        try {
          computeFinal();
        } catch (NonObjectJSONException e) {
          abort(Constants.JPAKE_ERROR_INVALID);
          return;
        }
      } else if (this.state == State.VERIFY_KEY) {
        decryptData(myKeyBundle);
      } else if (this.state == State.VERIFY_PAIRING) {
        try {
          if (verifyPairing(jIncoming, myKeyBundle)) {
            paired = true;
            ssActivity.onPaired();
          } else {
            abort(Constants.JPAKE_ERROR_KEYMISMATCH);
          }
        } catch (NonObjectJSONException e) {
          e.printStackTrace();
        } catch (CryptoException e) {
          e.printStackTrace();
        } catch (IOException e) {
          e.printStackTrace();
        } catch (ParseException e) {
          e.printStackTrace();
        }
      }
      break;

    case PUT:
      etagHeaders = response.httpResponse().getHeaders("etag");
      myEtag = response.httpResponse().getHeaders("etag")[0].getValue();

      state = stateContext;
      if (state == State.VERIFY_KEY) {
        jpakeMaxTries = MAX_TRIES_LAST_MSG;
        ssActivity.onPaired();
      }
      if (state == State.SNDR_STEP_ONE) {
        try {
          computeStepTwo();
        } catch (NonObjectJSONException e) {
          Log.e(LOG_TAG, "NonObjectJSONException", e);
          abort(Constants.JPAKE_ERROR_INVALID);
          return;
        }
        return; 
      }
      if (state == State.SNDR_STEP_TWO) {
        try {
          computeFinal();
        } catch (NonObjectJSONException e) {
          Log.e(LOG_TAG, "NonObjectJSONException", e);
          abort(Constants.JPAKE_ERROR_INVALID);
          return;
        }
        return; 
      }

      
      scheduleGetRequest(2 * jpakePollInterval);
      Log.i(LOG_TAG, "scheduling 2xPollInterval for " + state.name());
      break;

    case ENCRYPT_PUT:
      complete(null); 
                      
      break;

    case ABORT:
      Log.e(LOG_TAG, "Key exchange successfully aborted.");
      break;
    default:
      Log.e(LOG_TAG, "Unhandled response success.");
    }
  }

  
  public ResourceDelegate makeRequestResourceDelegate() {
    return new JPakeRequestResourceDelegate(this);
  }

  public class JPakeRequestResourceDelegate implements ResourceDelegate {

    private JPakeRequestDelegate requestDelegate;

    public JPakeRequestResourceDelegate(JPakeRequestDelegate delegate) {
      this.requestDelegate = delegate;
    }

    @Override
    public String getCredentials() {
      
      return null;
    }

    @Override
    public void addHeaders(HttpRequestBase request, DefaultHttpClient client) {
      request.setHeader(new BasicHeader("X-KeyExchange-Id", clientId));

      switch (state) {
      case REPORT_FAILURE:
        

      case ABORT:
        request.setHeader(new BasicHeader("X-KeyExchange-Cid", channel));
        break;

      case PUT:
        if (myEtag == null && state == State.RCVR_STEP_ONE) {
          request.setHeader(new BasicHeader("If-None-Match", "*"));
        }
        
      case VERIFY_KEY:
      case VERIFY_PAIRING:
      case RCVR_STEP_ONE:
      case RCVR_STEP_TWO:
        if (myEtag != null) {
          request.setHeader(new BasicHeader("If-None-Match", myEtag));
        }
        break;
      }
    }

    @Override
    public void handleHttpResponse(HttpResponse response) {
      
      if (isSuccess(response)) {
        this.requestDelegate.onRequestSuccess(response);
      } else {
        this.requestDelegate.onRequestFailure(response);
      }
      SyncResourceDelegate.consumeEntity(response.getEntity());
    }

    @Override
    public void handleHttpProtocolException(ClientProtocolException e) {
      Log.e(LOG_TAG, "Got HTTP protocol exception.", e);
      this.requestDelegate.onRequestError(e);
    }

    @Override
    public void handleTransportException(GeneralSecurityException e) {
      Log.e(LOG_TAG, "Got HTTP transport exception.", e);
      this.requestDelegate.onRequestError(e);
    }

    @Override
    public void handleHttpIOException(IOException e) {
      
      Log.e(LOG_TAG, "HttpIOException", e);
      switch (state) {
      case GET_CHANNEL:
        Log.e(LOG_TAG, "Failed on GetChannel.", e);
        break;

      case RCVR_STEP_ONE:
      case RCVR_STEP_TWO:
        Log.e(LOG_TAG, "Failed on GET", e);
        break;
      case PUT:
        Log.e(LOG_TAG, "Failed on PUT.", e);
        break;

      case REPORT_FAILURE:
        Log.e(LOG_TAG, "Report failed: " + error);
        break;
      default:
        Log.e(LOG_TAG, "Unhandled HTTP I/O exception.", e);
      }
      abort(Constants.JPAKE_ERROR_NETWORK);
    }

    @Override
    public int connectionTimeout() {
      return REQUEST_TIMEOUT;
    }

    @Override
    public int socketTimeout() {
      return SOCKET_TIMEOUT;
    }

    private int getStatusCode(HttpResponse response) {
      return response.getStatusLine().getStatusCode();
    }

    private boolean isSuccess(HttpResponse response) {
      return getStatusCode(response) == 200;
    }
  }

  








  public void abort(String error) {
    Logger.debug(LOG_TAG, "aborting...");
    finished = true;

    if (error == null) {
      error = Constants.JPAKE_ERROR_USERABORT;
    }
    Logger.debug(LOG_TAG, error);

    if (Constants.JPAKE_ERROR_CHANNEL.equals(error)
        || Constants.JPAKE_ERROR_NETWORK.equals(error)
        || Constants.JPAKE_ERROR_NODATA.equals(error)) {
      
    } else {
      reportFailure(error);
    }
    ssActivity.displayAbort(error);
  }

  


  private void reportFailure(String error) {
    Logger.debug(LOG_TAG, "reporting error to server");
    this.error = error;
    runOnThread(new Runnable() {
      @Override
      public void run() {
        JPakeRequest report;
        try {
          report = new JPakeRequest(jpakeServer + "report",
              makeRequestResourceDelegate());
          report.post(new StringEntity(""));
        } catch (URISyntaxException e) {
          e.printStackTrace();
        } catch (UnsupportedEncodingException e) {
          e.printStackTrace();
        }
      }
    });

  }

  

  


  private void setClientId() {
    byte[] rBytes = Utils.generateRandomBytes(JPAKE_LENGTH_CLIENTID / 2);
    StringBuilder id = new StringBuilder();

    for (byte b : rBytes) {
      String hexString = Integer.toHexString(b);
      if (hexString.length() == 1) {
        hexString = "0" + hexString;
      }
      int len = hexString.length();
      id.append(hexString.substring(len - 2, len));
    }
    clientId = id.toString();
  }

  


  private void createSecret() {
    
    String key = "23456789abcdefghijkmnpqrstuvwxyz";
    int keylen = key.length();

    byte[] rBytes = Utils.generateRandomBytes(JPAKE_LENGTH_SECRET);
    StringBuilder secret = new StringBuilder();
    for (byte b : rBytes) {
      secret.append(key.charAt(Math.abs(b) * keylen / 256));
    }
    this.secret = secret.toString();
  }

  






  protected StringEntity jsonEntity(JSONObject body)
      throws UnsupportedEncodingException {
    StringEntity e = new StringEntity(body.toJSONString(), "UTF-8");
    e.setContentType("application/json");
    return e;
  }

  



  public class GetStepTimerTask extends TimerTask {
    private JPakeRequest request;

    public GetStepTimerTask(JPakeRequest request) {
      this.request = request;
    }

    @Override
    public void run() {
      request.get();
    }
  }

  



  private void scheduleGetRequest(int delay) {
    JPakeRequest getRequest = null;
    try {
      getRequest = new JPakeRequest(channelUrl, makeRequestResourceDelegate());
    } catch (URISyntaxException e) {
      Log.e(LOG_TAG, "URISyntaxException", e);
      abort(Constants.JPAKE_ERROR_CHANNEL);
      return;
    }

    GetStepTimerTask getStepTimerTask = new GetStepTimerTask(getRequest);
    timerScheduler.schedule(getStepTimerTask, delay);
  }

  



  private JSONObject getJSONObject(String jsonString) throws Exception {
    Reader in = new StringReader(jsonString);
    try {
      return (JSONObject) new JSONParser().parse(in);
    } catch (Exception e) {
      throw e;
    }
  }

  


  public static BigInteger secretAsBigInteger(String secretString) throws UnsupportedEncodingException {
    return new BigInteger(secretString.getBytes("UTF-8"));
  }


  


  private ExtendedJSONObject makeJZkp(BigInteger gr, BigInteger b, String id) {
    ExtendedJSONObject result = new ExtendedJSONObject();
    result.put(Constants.ZKP_KEY_GR, BigIntegerHelper.toEvenLengthHex(gr));
    result.put(Constants.ZKP_KEY_B, BigIntegerHelper.toEvenLengthHex(b));
    result.put(Constants.ZKP_KEY_ID, id);
    return result;
  }

  








  public byte[] decryptPayload(ExtendedJSONObject payload, KeyBundle keybundle)
      throws CryptoException, UnsupportedEncodingException {
    byte[] ciphertext = Utils.decodeBase64((String) payload
        .get(Constants.JSON_KEY_CIPHERTEXT));
    byte[] iv = Utils.decodeBase64((String) payload.get(Constants.JSON_KEY_IV));
    byte[] hmac = Utils.hex2Byte((String) payload.get(Constants.JSON_KEY_HMAC));
    byte[] plainbytes = Cryptographer.decrypt(new CryptoInfo(ciphertext, iv,
        hmac, keybundle));
    return plainbytes;
  }

  









  public ExtendedJSONObject encryptPayload(String data, KeyBundle keyBundle)
      throws UnsupportedEncodingException, CryptoException {
    if (keyBundle == null) {
      throw new NoKeyBundleException();
    }
    byte[] cleartextBytes = data.getBytes("UTF-8");
    CryptoInfo info = new CryptoInfo(cleartextBytes, keyBundle);
    Cryptographer.encrypt(info);
    String message = new String(Base64.encodeBase64(info.getMessage()));
    String iv = new String(Base64.encodeBase64(info.getIV()));

    ExtendedJSONObject payload = new ExtendedJSONObject();
    payload.put(Constants.JSON_KEY_CIPHERTEXT, message);
    payload.put(Constants.JSON_KEY_IV, iv);
    if (this.state == State.ENCRYPT_PUT) {
      String hmac = Utils.byte2hex(info.getHMAC());
      payload.put(Constants.JSON_KEY_HMAC, hmac);
    }
    return payload;
  }
}

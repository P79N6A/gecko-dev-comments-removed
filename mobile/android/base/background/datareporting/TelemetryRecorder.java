



package org.mozilla.gecko.background.datareporting;

import java.io.BufferedOutputStream;
import java.io.File;
import java.io.FileOutputStream;
import java.io.IOException;
import java.io.OutputStream;
import java.io.UnsupportedEncodingException;
import java.security.MessageDigest;
import java.security.NoSuchAlgorithmException;

import org.json.JSONObject;
import org.mozilla.gecko.background.common.log.Logger;

import android.util.Base64;

























public class TelemetryRecorder {
  private final String LOG_TAG = "TelemetryRecorder";

  private final File parentDir;
  private final String filename;

  private File tmpFile;
  private File destFile;
  private File cacheDir;

  private OutputStream  outputStream;
  private MessageDigest checksum;
  private String        base64Checksum;

  







  private String charset = "us-ascii";

  



  private int blockSize = 0;

  


















  public TelemetryRecorder(File parentDir, File cacheDir, String filename) {
    if (!parentDir.isDirectory()) {
      throw new IllegalArgumentException("Expecting directory, got non-directory File instead.");
    }
    this.parentDir = parentDir;
    this.filename = filename;
    this.cacheDir = cacheDir;
  }

  public TelemetryRecorder(File parentDir, File cacheDir, String filename, String charset) {
    this(parentDir, cacheDir, filename);
    this.charset = charset;
  }

  public TelemetryRecorder(File parentDir, File cacheDir, String filename, String charset, int blockSize) {
    this(parentDir, cacheDir, filename, charset);
    this.blockSize = blockSize;
  }

  











  public void startPingFile() throws Exception {

    
    try {
      tmpFile = File.createTempFile(filename, "tmp", cacheDir);
    } catch (IOException e) {
      
      tmpFile = new File(parentDir, filename + ".tmp");
      try {
        tmpFile.createNewFile();
      } catch (IOException e1) {
        cleanUpAndRethrow("Failed to create tmp file in temp directory or ping directory.", e1);
      }
    }

    try {
      if (blockSize > 0) {
        outputStream = new BufferedOutputStream(new FileOutputStream(tmpFile), blockSize);
      } else {
        outputStream = new BufferedOutputStream(new FileOutputStream(tmpFile));
      }

      
      checksum = MessageDigest.getInstance("SHA-256");

      
      byte[] header = makePingHeader(filename);
      outputStream.write(header);
      Logger.debug(LOG_TAG, "Wrote " + header.length + " header bytes.");

    } catch (NoSuchAlgorithmException e) {
      cleanUpAndRethrow("Error creating checksum digest", e);
    } catch (UnsupportedEncodingException e) {
      cleanUpAndRethrow("Error writing header", e);
    } catch (IOException e) {
      cleanUpAndRethrow("Error writing to stream", e);
    }
  }

  private byte[] makePingHeader(String slug)
      throws UnsupportedEncodingException {
    return ("{\"slug\":" + JSONObject.quote(slug) + "," + "\"payload\":\"")
        .getBytes(charset);
  }

  










  public int appendPayload(String payloadContent) throws Exception {
    if (payloadContent == null) {
      cleanUpAndRethrow("Payload is null", new Exception());
      return -1;
    }

    try {
      byte[] payloadBytes = payloadContent.getBytes(charset);
      
      checksum.update(payloadBytes);

      byte[] quotedPayloadBytes = JSONObject.quote(payloadContent).getBytes(charset);

      
      
      int numBytes = quotedPayloadBytes.length - 2;
      outputStream.write(quotedPayloadBytes, 1, numBytes);
      return numBytes;

    } catch (UnsupportedEncodingException e) {
      cleanUpAndRethrow("Error encoding payload", e);
      return -1;
    } catch (IOException e) {
      cleanUpAndRethrow("Error writing to stream", e);
      return -1;
    }
  }

  







  public void finishPingFile() throws Exception {
    try {
      byte[] footer = makePingFooter(checksum);
      outputStream.write(footer);
      
      outputStream.flush();
      Logger.debug(LOG_TAG, "Wrote " + footer.length + " footer bytes.");
    } catch (UnsupportedEncodingException e) {
      cleanUpAndRethrow("Checksum encoding exception", e);
    } catch (IOException e) {
      cleanUpAndRethrow("Error writing footer to stream", e);
    } finally {
      try {
        outputStream.close();
      } catch (IOException e) {
        
        outputStream = null;
      }
    }

    
    try {
      File destFile = new File(parentDir, filename);
      
      if (destFile.exists()) {
        destFile.delete();
      }
      boolean result = tmpFile.renameTo(destFile);
      if (!result) {
        throw new IOException("Could not move tmp file to destination.");
      }
    } finally {
      cleanUp();
    }
  }

  private byte[] makePingFooter(MessageDigest checksum)
      throws UnsupportedEncodingException {
    base64Checksum = Base64.encodeToString(checksum.digest(), Base64.NO_WRAP);
    return ("\",\"checksum\":" + JSONObject.quote(base64Checksum) + "}")
        .getBytes(charset);
  }

  




  protected String getFinalChecksum() {
    return base64Checksum;
  }

  public String getCharset() {
    return this.charset;
  }

  


  private void cleanUp() {
    
    checksum.reset();

    
    if (tmpFile != null && tmpFile.exists()) {
      tmpFile.delete();
    }
    tmpFile = null;
  }

  










  private void cleanUpAndRethrow(String message, Exception e) throws Exception {
    Logger.error(LOG_TAG, message, e);
    cleanUp();

    if (outputStream != null) {
      try {
        outputStream.close();
      } catch (IOException exception) {
        
      }
    }

    if (destFile != null && destFile.exists()) {
      destFile.delete();
    }
    
    throw e;
  }
}

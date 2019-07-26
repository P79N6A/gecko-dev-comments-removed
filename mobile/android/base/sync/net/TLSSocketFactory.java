



package org.mozilla.gecko.sync.net;

import java.io.IOException;
import java.net.Socket;

import javax.net.ssl.SSLContext;
import javax.net.ssl.SSLSocket;

import org.mozilla.gecko.background.common.log.Logger;

import ch.boye.httpclientandroidlib.conn.ssl.SSLSocketFactory;
import ch.boye.httpclientandroidlib.params.HttpParams;

public class TLSSocketFactory extends SSLSocketFactory {
  private static final String LOG_TAG = "TLSSocketFactory";
  private static final String[] DEFAULT_CIPHER_SUITES = new String[] {
    "SSL_RSA_WITH_RC4_128_SHA",        
  };
  private static final String[] DEFAULT_PROTOCOLS = new String[] {
    "SSLv3",
    "TLSv1"
  };

  
  private static String[] cipherSuites = DEFAULT_CIPHER_SUITES;

  public TLSSocketFactory(SSLContext sslContext) {
    super(sslContext);
  }

  














  public static synchronized void setEnabledCipherSuites(SSLSocket socket) {
    try {
      socket.setEnabledCipherSuites(cipherSuites);
    } catch (IllegalArgumentException e) {
      cipherSuites = socket.getSupportedCipherSuites();
      Logger.warn(LOG_TAG, "Setting enabled cipher suites failed: " + e.getMessage());
      Logger.warn(LOG_TAG, "Using " + cipherSuites.length + " supported suites.");
      socket.setEnabledCipherSuites(cipherSuites);
    }
  }

  @Override
  public Socket createSocket(HttpParams params) throws IOException {
    SSLSocket socket = (SSLSocket) super.createSocket(params);
    socket.setEnabledProtocols(DEFAULT_PROTOCOLS);
    setEnabledCipherSuites(socket);
    return socket;
  }
}

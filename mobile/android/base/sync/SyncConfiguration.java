




































package org.mozilla.gecko.sync;

import java.net.URI;
import java.net.URISyntaxException;

import org.mozilla.gecko.sync.crypto.KeyBundle;

import android.util.Log;

public class SyncConfiguration implements CredentialsSource {
  public static final String DEFAULT_USER_API = "https://auth.services.mozilla.com/user/1.0/";

  private static final String LOG_TAG = "SyncConfiguration";

  
  public String          userAPI;
  public URI             serverURL;
  public URI             clusterURL;
  public String          username;
  public KeyBundle       syncKeyBundle;

  public CollectionKeys  collectionKeys;
  public InfoCollections infoCollections;
  public MetaGlobal      metaGlobal;
  public String          password;
  public String          syncID;


  public SyncConfiguration() {
  }

  @Override
  public String credentials() {
    return username + ":" + password;
  }

  @Override
  public CollectionKeys getCollectionKeys() {
    return collectionKeys;
  }

  @Override
  public KeyBundle keyForCollection(String collection) throws NoCollectionKeysSetException {
    return getCollectionKeys().keyBundleForCollection(collection);
  }

  public void setCollectionKeys(CollectionKeys k) {
    collectionKeys = k;
  }

  public String nodeWeaveURL() {
    return this.nodeWeaveURL((this.serverURL == null) ? null : this.serverURL.toASCIIString());
  }

  public String nodeWeaveURL(String serverURL) {
    String userPart = username + "/node/weave";
    if (serverURL == null) {
      return DEFAULT_USER_API + userPart;
    }
    if (!serverURL.endsWith("/")) {
      serverURL = serverURL + "/";
    }
    return serverURL + "user/1.0/" + userPart;
  }

  public String infoURL() {
    return clusterURL + GlobalSession.API_VERSION + "/" + username + "/info/collections";
  }
  public String metaURL() {
    return clusterURL + GlobalSession.API_VERSION + "/" + username + "/storage/meta/global";
  }

  public String storageURL(boolean trailingSlash) {
    return clusterURL + GlobalSession.API_VERSION + "/" + username +
           (trailingSlash ? "/storage/" : "/storage");
  }

  public URI collectionURI(String collection, boolean full) throws URISyntaxException {
    
    
    boolean anyParams = full;
    String  uriParams = "";
    if (anyParams) {
      StringBuilder params = new StringBuilder("?");
      if (full) {
        params.append("full=1");
      }
      uriParams = params.toString();
    }
    String uri = storageURL(true) + collection + uriParams;
    return new URI(uri);
  }

  public URI wboURI(String collection, String id) throws URISyntaxException {
    return new URI(storageURL(true) + collection + "/" + id);
  }

  public URI keysURI() throws URISyntaxException {
    return wboURI("crypto", "keys");
  }

  public void setClusterURL(URI u) {
    if (u == null) {
      Log.w(LOG_TAG, "Refusing to set cluster URL to null.");
      return;
    }
    URI uri = u.normalize();
    if (uri.toASCIIString().endsWith("/")) {
      this.clusterURL = u;
      return;
    }
    this.clusterURL = uri.resolve("/");
    Log.i(LOG_TAG, "Set cluster URL to " + this.clusterURL.toASCIIString() + ", given input " + u.toASCIIString());
  }

  public void setClusterURL(String url) throws URISyntaxException {
    this.setClusterURL(new URI(url));
  }
}

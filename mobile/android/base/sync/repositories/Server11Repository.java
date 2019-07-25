




































package org.mozilla.gecko.sync.repositories;

import java.net.URI;
import java.net.URISyntaxException;

import org.mozilla.gecko.sync.CredentialsSource;
import org.mozilla.gecko.sync.repositories.delegates.RepositorySessionCreationDelegate;

import android.content.Context;







public class Server11Repository extends Repository {

  private String serverURI;
  private String username;
  private String collection;
  private String collectionPath;
  public CredentialsSource credentialsSource;
  public static final String VERSION_PATH_FRAGMENT = "1.1/";

  








  public Server11Repository(String serverURI, String username, String collection, CredentialsSource credentialsSource) {
    this.serverURI  = serverURI;
    this.username   = username;
    this.collection = collection;

    this.collectionPath = this.serverURI + VERSION_PATH_FRAGMENT + this.username + "/storage/" + this.collection;
    this.credentialsSource = credentialsSource;
  }

  @Override
  public void createSession(RepositorySessionCreationDelegate delegate,
                            Context context) {
    delegate.onSessionCreated(new Server11RepositorySession(this));
  }

  public URI collectionURI(boolean full, long newer, String ids) throws URISyntaxException {
    
    
    
    boolean anyParams = full;
    String  uriParams = "";
    if (anyParams) {
      StringBuilder params = new StringBuilder("?");
      if (full) {
        params.append("full=1");
      }
      if (newer >= 0) {
        params.append((full ? "&newer=" : "newer=") + newer);
      }
      if (ids != null) {
        params.append(((full || newer >= 0) ? "&ids=" : "ids=") + ids);
      }
      uriParams = params.toString();
    }
    String uri = this.collectionPath + uriParams;
    return new URI(uri);
  }

  public URI wboURI(String id) throws URISyntaxException {
    return new URI(this.collectionPath + "/" + id);
  }
}

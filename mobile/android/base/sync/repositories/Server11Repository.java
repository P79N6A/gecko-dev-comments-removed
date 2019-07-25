




































package org.mozilla.gecko.sync.repositories;

import java.net.URI;
import java.net.URISyntaxException;
import java.util.ArrayList;

import org.mozilla.gecko.sync.CredentialsSource;
import org.mozilla.gecko.sync.Utils;
import org.mozilla.gecko.sync.repositories.delegates.RepositorySessionCreationDelegate;

import android.content.Context;







public class Server11Repository extends Repository {

  private String serverURI;
  private String username;
  private String collection;
  private String collectionPath;
  private URI collectionPathURI;
  public CredentialsSource credentialsSource;
  public static final String VERSION_PATH_FRAGMENT = "1.1/";

  









  public Server11Repository(String serverURI, String username, String collection, CredentialsSource credentialsSource) throws URISyntaxException {
    this.serverURI  = serverURI;
    this.username   = username;
    this.collection = collection;

    this.collectionPath = this.serverURI + VERSION_PATH_FRAGMENT + this.username + "/storage/" + this.collection;
    this.collectionPathURI = new URI(this.collectionPath);
    this.credentialsSource = credentialsSource;
  }

  @Override
  public void createSession(RepositorySessionCreationDelegate delegate,
                            Context context) {
    delegate.onSessionCreated(new Server11RepositorySession(this));
  }

  public URI collectionURI() {
    return this.collectionPathURI;
  }

  public URI collectionURI(boolean full, long newer, long limit, String sort, String ids) throws URISyntaxException {
    ArrayList<String> params = new ArrayList<String>();
    if (full) {
      params.add("full=1");
    }
    if (newer >= 0) {
      
      String newerString = Utils.millisecondsToDecimalSecondsString(newer);
      params.add("newer=" + newerString);
    }
    if (limit > 0) {
      params.add("limit=" + limit);
    }
    if (sort != null) {
      params.add("sort=" + sort);       
    }
    if (ids != null) {
      params.add("ids=" + ids);         
    }

    if (params.size() == 0) {
      return this.collectionPathURI;
    }

    StringBuilder out = new StringBuilder();
    char indicator = '?';
    for (String param : params) {
      out.append(indicator);
      indicator = '&';
      out.append(param);
    }
    String uri = this.collectionPath + out.toString();
    return new URI(uri);
  }

  public URI wboURI(String id) throws URISyntaxException {
    return new URI(this.collectionPath + "/" + id);
  }

  
  protected long getDefaultFetchLimit() {
    return -1;
  }
  protected String getDefaultSort() {
    return null;
  }
}

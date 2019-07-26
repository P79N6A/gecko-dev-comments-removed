



package org.mozilla.gecko.sync.repositories;

import java.net.URI;
import java.net.URISyntaxException;
import java.util.ArrayList;

import org.mozilla.gecko.sync.Utils;
import org.mozilla.gecko.sync.net.AuthHeaderProvider;
import org.mozilla.gecko.sync.repositories.delegates.RepositorySessionCreationDelegate;

import android.content.Context;







public class Server11Repository extends Repository {
  protected String collection;
  protected URI collectionURI;
  protected final AuthHeaderProvider authHeaderProvider;
  public static final String VERSION_PATH_FRAGMENT = "1.1/";

  







  public Server11Repository(String collection, String storageURL, AuthHeaderProvider authHeaderProvider) throws URISyntaxException {
    this.collection = collection;
    this.collectionURI = new URI(storageURL + (storageURL.endsWith("/") ? collection : "/" + collection));
    this.authHeaderProvider = authHeaderProvider;
  }

  @Override
  public void createSession(RepositorySessionCreationDelegate delegate,
                            Context context) {
    delegate.onSessionCreated(new Server11RepositorySession(this));
  }

  public URI collectionURI() {
    return this.collectionURI;
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
      return this.collectionURI;
    }

    StringBuilder out = new StringBuilder();
    char indicator = '?';
    for (String param : params) {
      out.append(indicator);
      indicator = '&';
      out.append(param);
    }
    String uri = this.collectionURI + out.toString();
    return new URI(uri);
  }

  public URI wboURI(String id) throws URISyntaxException {
    return new URI(this.collectionURI + "/" + id);
  }

  
  @SuppressWarnings("static-method")
  protected long getDefaultFetchLimit() {
    return -1;
  }

  @SuppressWarnings("static-method")
  protected String getDefaultSort() {
    return null;
  }

  public AuthHeaderProvider getAuthHeaderProvider() {
    return authHeaderProvider;
  }
}

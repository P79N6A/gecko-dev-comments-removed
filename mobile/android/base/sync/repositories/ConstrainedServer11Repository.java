



package org.mozilla.gecko.sync.repositories;

import java.net.URISyntaxException;

import org.mozilla.gecko.sync.net.AuthHeaderProvider;







public class ConstrainedServer11Repository extends Server11Repository {

  private String sort = null;
  private long limit  = -1;

  public ConstrainedServer11Repository(String collection, String storageURL, AuthHeaderProvider authHeaderProvider, long limit, String sort) throws URISyntaxException {
    super(collection, storageURL, authHeaderProvider);
    this.limit = limit;
    this.sort  = sort;
  }

  @Override
  protected String getDefaultSort() {
    return sort;
  }

  @Override
  protected long getDefaultFetchLimit() {
    return limit;
  }
}

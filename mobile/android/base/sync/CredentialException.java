



package org.mozilla.gecko.sync;

import android.content.SyncResult;





public abstract class CredentialException extends SyncException {
  private static final long serialVersionUID = 833010553314100538L;

  public CredentialException() {
    super();
  }

  public CredentialException(final Throwable e) {
    super(e);
  }

  public void updateStats(GlobalSession globalSession, SyncResult syncResult) {
    syncResult.stats.numAuthExceptions += 1;
  }

  


  public static class MissingAllCredentialsException extends CredentialException {
    private static final long serialVersionUID = 3763937096217604611L;

    public MissingAllCredentialsException() {
      super();
    }

    public MissingAllCredentialsException(final Throwable e) {
      super(e);
    }
  }

  


  public static class MissingCredentialException extends CredentialException {
    private static final long serialVersionUID = -7543031216547596248L;

    public final String missingCredential;

    public MissingCredentialException(final String missingCredential) {
      this.missingCredential = missingCredential;
    }
  }
}






































package org.mozilla.gecko.sync.repositories.android;

import org.mozilla.gecko.sync.repositories.domain.PasswordRecord;
import org.mozilla.gecko.sync.repositories.domain.Record;

import android.content.ContentValues;
import android.content.Context;
import android.net.Uri;

public class AndroidBrowserPasswordsDataAccessor extends AndroidBrowserRepositoryDataAccessor {

  public AndroidBrowserPasswordsDataAccessor(Context context) {
    super(context);
  }

  @Override
  protected ContentValues getContentValues(Record record) {
    ContentValues cv = new ContentValues();
    PasswordRecord rec = (PasswordRecord) record;
    cv.put(PasswordColumns.HOSTNAME, rec.hostname);
    cv.put(PasswordColumns.HTTP_REALM, rec.httpRealm);
    cv.put(PasswordColumns.FORM_SUBMIT_URL, rec.formSubmitURL);
    cv.put(PasswordColumns.USERNAME_FIELD, rec.usernameField);
    cv.put(PasswordColumns.PASSWORD_FIELD, rec.passwordField);
    cv.put(PasswordColumns.GUID,          rec.guid);
    
    
    
    cv.put(PasswordColumns.ENC_TYPE, rec.encType);
    cv.put(PasswordColumns.ENCRYPTED_USERNAME, rec.username);
    cv.put(PasswordColumns.ENCRYPTED_PASSWORD, rec.password);
    
    cv.put(PasswordColumns.TIMES_USED, rec.timesUsed);
    cv.put(PasswordColumns.TIME_LAST_USED, rec.timeLastUsed);
    return cv;
  }

  @Override
  protected Uri getUri() {
    return BrowserContract.Passwords.CONTENT_URI;
  }

  @Override
  protected String[] getAllColumns() {
    return BrowserContract.Passwords.PasswordColumns;
  }
  
}

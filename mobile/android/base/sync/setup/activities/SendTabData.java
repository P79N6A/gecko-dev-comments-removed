



package org.mozilla.gecko.sync.setup.activities;

import java.util.ArrayList;
import java.util.List;

import android.content.Intent;
import android.os.Bundle;







public class SendTabData {
  public final String title;
  public final String uri;

  public SendTabData(String title, String uri) {
    this.title = title;
    this.uri = uri;
  }

  public static SendTabData fromIntent(Intent intent) {
    if (intent == null) {
      throw new IllegalArgumentException("intent must not be null");
    }

    return fromBundle(intent.getExtras());
  }

  protected static SendTabData fromBundle(Bundle bundle) {
    if (bundle == null) {
      throw new IllegalArgumentException("bundle must not be null");
    }

    String text = bundle.getString(Intent.EXTRA_TEXT);
    String subject = bundle.getString(Intent.EXTRA_SUBJECT);
    String title = bundle.getString(Intent.EXTRA_TITLE);

    
    String theTitle = subject;
    if (theTitle == null) {
      theTitle = title;
    }

    
    
    List<String> strings = new ArrayList<String>();
    strings.add(text);
    strings.add(subject);
    strings.add(title);
    String theUri = new WebURLFinder(strings).bestWebURL();

    return new SendTabData(theTitle, theUri);
  }
}

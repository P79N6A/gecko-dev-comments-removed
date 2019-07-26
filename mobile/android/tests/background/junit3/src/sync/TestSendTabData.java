


package org.mozilla.gecko.background.sync;

import org.mozilla.gecko.background.helpers.AndroidSyncTestCase;
import org.mozilla.gecko.sync.setup.activities.SendTabData;

import android.content.Intent;





public class TestSendTabData extends AndroidSyncTestCase {
  protected static Intent makeShareIntent(String text, String subject, String title) {
    Intent intent = new Intent();

    intent.putExtra(Intent.EXTRA_TEXT, text);
    intent.putExtra(Intent.EXTRA_SUBJECT, subject);
    intent.putExtra(Intent.EXTRA_TITLE, title);

    return intent;
  }

  
  
  
  
  
  public void testFennecBrowser() {
    Intent shareIntent = makeShareIntent("http://www.reddit.com/",
        "reddit: the front page of the internet",
        null);
    SendTabData fromIntent = SendTabData.fromIntent(shareIntent);

    assertEquals("reddit: the front page of the internet", fromIntent.title);
    assertEquals("http://www.reddit.com/", fromIntent.uri);
  }

  
  
  
  
  
  public void testAndroidBrowser() {
    Intent shareIntent = makeShareIntent("http://www.reddit.com/",
        "reddit: the front page of the internet",
        null);
    SendTabData fromIntent = SendTabData.fromIntent(shareIntent);

    assertEquals("reddit: the front page of the internet", fromIntent.title);
    assertEquals("http://www.reddit.com/", fromIntent.uri);
  }

  
  
  
  
  
  
  public void testPocket() {
    Intent shareIntent = makeShareIntent("http://t.co/bfsbM2oV",
        "Launching the Canadian OGP Civil Society Discussion Group",
        "Launching the Canadian OGP Civil Society Discussion Group");
    SendTabData fromIntent = SendTabData.fromIntent(shareIntent);

    assertEquals("Launching the Canadian OGP Civil Society Discussion Group", fromIntent.title);
    assertEquals("http://t.co/bfsbM2oV", fromIntent.uri);
  }

  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  public void testTwitter() {
    Intent shareIntent1 = makeShareIntent("Cory Doctorow (@doctorow) tweeted at 11:21 AM on Sat, Jan 12, 2013:\n" +
        "Pls RT: @lessig on the DoJ's vindictive prosecution of Aaron Swartz http://t.co/qNalE70n #aaronsw\n" +
        "(https://twitter.com/doctorow/status/290176681065451520)\n" +
        "\n" +
        "Get the official Twitter app at https://twitter.com/download",
        "Tweet from Cory Doctorow (@doctorow)",
        null);
    SendTabData fromIntent1 = SendTabData.fromIntent(shareIntent1);

    assertEquals("Tweet from Cory Doctorow (@doctorow)", fromIntent1.title);
    assertEquals("http://t.co/qNalE70n", fromIntent1.uri);

    Intent shareIntent2 = makeShareIntent("David Eaves (@daeaves) tweeted at 0:08 PM on Fri, Jan 11, 2013:\n" +
        "New on eaves.ca: Launching the Canadian OGP Civil Society Discussion Group http://t.co/bfsbM2oV\n" +
        "(https://twitter.com/daeaves/status/289826143723466752)\n" +
        "\n" +
        "Get the official Twitter app at https://twitter.com/download",
        "Tweet from David Eaves (@daeaves)",
        null);
    SendTabData fromIntent2 = SendTabData.fromIntent(shareIntent2);

    assertEquals("Tweet from David Eaves (@daeaves)", fromIntent2.title);
    assertEquals("http://t.co/bfsbM2oV", fromIntent2.uri);
  }
}

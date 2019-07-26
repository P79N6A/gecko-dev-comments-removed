



package org.mozilla.gecko.sync.setup.activities;

import org.mozilla.gecko.R;
import org.mozilla.gecko.background.common.log.Logger;

import android.app.Activity;
import android.net.Uri;
import android.os.Bundle;
import android.view.Window;
import android.webkit.WebChromeClient;
import android.webkit.WebView;
import android.webkit.WebViewClient;






public class WebViewActivity extends SyncActivity {
  private final static String LOG_TAG = "WebViewActivity";

  @Override
  public void onCreate(Bundle savedInstanceState) {
    super.onCreate(savedInstanceState);
    getWindow().requestFeature(Window.FEATURE_PROGRESS);
    setContentView(R.layout.sync_setup_webview);
    
    Uri uri = this.getIntent().getData();
    if (uri == null) {
      Logger.debug(LOG_TAG, "No URI passed to display.");
      finish();
    }

    WebView wv = (WebView) findViewById(R.id.web_engine);
    
    final Activity activity = this;
    wv.setWebChromeClient(new WebChromeClient() {
      public void onProgressChanged(WebView view, int progress) {
        
        
        activity.setProgress(progress * 100);
      }
    });
    wv.setWebViewClient(new WebViewClient() {
      
      @Override
      public boolean shouldOverrideUrlLoading(WebView view, String url) {
        view.loadUrl(url);
        return false;
      }
    });
    wv.loadUrl(uri.toString());

  }
}

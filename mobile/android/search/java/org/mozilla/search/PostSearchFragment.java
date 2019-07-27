



package org.mozilla.search;

import android.content.Intent;
import android.graphics.Bitmap;
import android.net.Uri;
import android.os.Bundle;
import android.support.v4.app.Fragment;
import android.text.TextUtils;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.webkit.WebChromeClient;
import android.webkit.WebView;
import android.webkit.WebViewClient;
import android.widget.ProgressBar;

import org.mozilla.gecko.AppConstants;
import org.mozilla.gecko.Telemetry;
import org.mozilla.gecko.TelemetryContract;

public class PostSearchFragment extends Fragment {

    private static final String LOGTAG = "PostSearchFragment";
    private static final String ABOUT_BLANK = "about:blank";

    private ProgressBar progressBar;
    private WebView webview;

    private static final String HIDE_BANNER_SCRIPT = "javascript:(function(){var tag=document.createElement('style');" +
            "tag.type='text/css';document.getElementsByTagName('head')[0].appendChild(tag);tag.innerText='#nav,#header{display:none}'})();";

    public PostSearchFragment() {
    }

    @Override
    public View onCreateView(LayoutInflater inflater, ViewGroup container,
                             Bundle savedInstanceState) {
        View mainView = inflater.inflate(R.layout.search_fragment_post_search, container, false);

        progressBar = (ProgressBar) mainView.findViewById(R.id.progress_bar);

        webview = (WebView) mainView.findViewById(R.id.webview);
        webview.setWebChromeClient(new ChromeClient());
        webview.setWebViewClient(new LinkInterceptingClient());
        
        webview.getSettings().setJavaScriptEnabled(true);

        return mainView;
    }

    @Override
    public void onDestroyView() {
        super.onDestroyView();
        webview.setWebChromeClient(null);
        webview.setWebViewClient(null);
        webview = null;
        progressBar = null;
    }

    








    private boolean shouldSendToBrowser(String url) {
        return !(TextUtils.equals(ABOUT_BLANK, url) || url.contains(Constants.YAHOO_WEB_SEARCH_RESULTS_FILTER));
    }

    public void startSearch(String query) {
        setUrl(Constants.YAHOO_WEB_SEARCH_BASE_URL + Uri.encode(query));
    }

    private void setUrl(String url) {
        
        
        if (!TextUtils.equals(webview.getUrl(), url)) {
            webview.loadUrl(ABOUT_BLANK);
            webview.loadUrl(url);
        }
    }

    




    private class LinkInterceptingClient extends WebViewClient {

        @Override
        public void onPageStarted(WebView view, String url, Bitmap favicon) {
            if (shouldSendToBrowser(url)) {
                view.stopLoading();

                Telemetry.sendUIEvent(TelemetryContract.Event.LOAD_URL,
                        TelemetryContract.Method.CONTENT, "search-result");

                final Intent i = new Intent(Intent.ACTION_VIEW);
                i.setClassName(AppConstants.ANDROID_PACKAGE_NAME, AppConstants.BROWSER_INTENT_CLASS_NAME);
                i.setData(Uri.parse(url));
                startActivity(i);
            }
        }
    }

    







    private class ChromeClient extends WebChromeClient {

        @Override
        public void onReceivedTitle(WebView view, String title) {
            super.onReceivedTitle(view, title);
            view.loadUrl(HIDE_BANNER_SCRIPT);
        }

        @Override
        public void onProgressChanged(WebView view, int newProgress) {
            if (newProgress < 100) {
                if (progressBar.getVisibility() == View.INVISIBLE) {
                    progressBar.setVisibility(View.VISIBLE);
                }
                progressBar.setProgress(newProgress);
            } else {
                progressBar.setVisibility(View.INVISIBLE);
            }
        }
    }
}

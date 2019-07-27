



package org.mozilla.search;

import android.app.Activity;
import android.content.Intent;
import android.graphics.Bitmap;
import android.net.Uri;
import android.os.Bundle;
import android.provider.Settings;
import android.support.v4.app.Fragment;
import android.text.TextUtils;
import android.util.Log;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.view.ViewStub;
import android.webkit.WebChromeClient;
import android.webkit.WebView;
import android.webkit.WebViewClient;
import android.widget.ImageView;
import android.widget.ProgressBar;
import android.widget.TextView;

import org.mozilla.gecko.AppConstants;
import org.mozilla.gecko.Telemetry;
import org.mozilla.gecko.TelemetryContract;
import org.mozilla.search.providers.SearchEngine;
import org.mozilla.search.providers.SearchEngineManager;

public class PostSearchFragment extends Fragment {

    private static final String LOG_TAG = "PostSearchFragment";

    private ProgressBar progressBar;

    private SearchEngineManager searchEngineManager;
    private WebView webview;
    private View errorView;

    @Override
    public View onCreateView(LayoutInflater inflater, ViewGroup container,
                             Bundle savedInstanceState) {
        View mainView = inflater.inflate(R.layout.search_fragment_post_search, container, false);

        progressBar = (ProgressBar) mainView.findViewById(R.id.progress_bar);

        webview = (WebView) mainView.findViewById(R.id.webview);
        webview.setWebChromeClient(new ChromeClient());
        webview.setWebViewClient(new ResultsWebViewClient());

        
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

    @Override
    public void onAttach(Activity activity) {
        super.onAttach(activity);
        searchEngineManager = new SearchEngineManager(activity);
    }

    @Override
    public void onDetach() {
        super.onDetach();
        searchEngineManager.destroy();
        searchEngineManager = null;
    }

    public void startSearch(final String query) {
        searchEngineManager.getEngine(new SearchEngineManager.SearchEngineCallback() {
            @Override
            public void execute(SearchEngine engine) {
                final String url = engine.resultsUriForQuery(query);

                
                webview.loadUrl(Constants.ABOUT_BLANK);
                webview.loadUrl(url);
            }
        });
    }


    




    private class ResultsWebViewClient extends WebViewClient {

        
        private boolean networkError;

        @Override
        public void onPageStarted(WebView view, final String url, Bitmap favicon) {
            
            networkError = false;

            searchEngineManager.getEngine(new SearchEngineManager.SearchEngineCallback() {
                @Override
                public void execute(SearchEngine engine) {
                    
                    if (TextUtils.equals(url, Constants.ABOUT_BLANK) || engine.isSearchResultsPage(url)) {
                        
                        return;
                    }

                    webview.stopLoading();

                    Telemetry.sendUIEvent(TelemetryContract.Event.LOAD_URL,
                            TelemetryContract.Method.CONTENT, "search-result");

                    final Intent i = new Intent(Intent.ACTION_VIEW);

                    
                    i.setClassName(AppConstants.ANDROID_PACKAGE_NAME, AppConstants.BROWSER_INTENT_CLASS_NAME);
                    i.setData(Uri.parse(url));
                    startActivity(i);
                }
            });
        }

        @Override
        public void onReceivedError(WebView view, int errorCode, String description, String failingUrl) {
            Log.e(LOG_TAG, "Error loading search results: " + description);

            networkError = true;

            if (errorView == null) {
                final ViewStub errorViewStub = (ViewStub) getView().findViewById(R.id.error_view_stub);
                errorView = errorViewStub.inflate();

                ((ImageView) errorView.findViewById(R.id.empty_image)).setImageResource(R.drawable.network_error);
                ((TextView) errorView.findViewById(R.id.empty_title)).setText(R.string.network_error_title);

                final TextView message = (TextView) errorView.findViewById(R.id.empty_message);
                message.setText(R.string.network_error_message);
                message.setTextColor(getResources().getColor(R.color.network_error_link));
                message.setOnClickListener(new View.OnClickListener() {
                    @Override
                    public void onClick(View v) {
                        startActivity(new Intent(Settings.ACTION_SETTINGS));
                    }
                });
            }
        }

        @Override
        public void onPageFinished(WebView view, String url) {
            
            if (errorView != null) {
                errorView.setVisibility(networkError ? View.VISIBLE : View.GONE);
                webview.setVisibility(networkError ? View.GONE : View.VISIBLE);
            }
        }
    }

    







    private class ChromeClient extends WebChromeClient {

        @Override
        public void onReceivedTitle(final WebView view, String title) {

            searchEngineManager.getEngine(new SearchEngineManager.SearchEngineCallback() {
                @Override
                public void execute(SearchEngine engine) {
                    view.loadUrl(engine.getInjectableJs());
                }
            });
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

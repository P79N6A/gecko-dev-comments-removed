



package org.mozilla.search.providers;

import android.net.Uri;

public class GoogleSearchEngine extends SearchEngine {

    private static final String HIDE_BANNER_CSS = "#sfcnt,#top_nav{display:none}";

    private static final Uri RESULTS_URI = Uri.parse("https://www.google.com/search");
    private static final String RESULTS_URI_QUERY_PARAM = "q";

    private static final Uri SUGGEST_URI = Uri.parse("https://www.google.com/complete/search?client=firefox");
    private static final String SUGGEST_URI_QUERY_PARAM = "q";

    @Override
    public String getInjectableCss() {
        return HIDE_BANNER_CSS;
    }

    @Override
    protected Uri getResultsUri() {
        return RESULTS_URI;
    }

    @Override
    protected Uri getSuggestionUri() {
        return SUGGEST_URI;
    }

    @Override
    protected String getSuggestionQueryParam() {
        return SUGGEST_URI_QUERY_PARAM;
    }

    @Override
    protected String getResultsQueryParam() {
        return RESULTS_URI_QUERY_PARAM;
    }

}

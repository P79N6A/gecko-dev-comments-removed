



package org.mozilla.search.providers;

import android.net.Uri;





public abstract class SearchEngine {

    
    
    private static final String STYLE_INJECTION_SCRIPT =
            "javascript:(function(){" +
                    "var tag=document.createElement('style');" +
                    "tag.type='text/css';" +
                    "document.getElementsByTagName('head')[0].appendChild(tag);" +
                    "tag.innerText='%s'})();";

    private String suggestionTemplate;

    



    public String getInjectableJs() {
        return String.format(STYLE_INJECTION_SCRIPT, getInjectableCss());
    }

    



    final public boolean isSearchResultsPage(String url) {
        return getResultsUri().getAuthority().equalsIgnoreCase(Uri.parse(url).getAuthority());
    }

    




    final public String suggestUriForQuery(String query) {
        return getSuggestionUri().buildUpon().appendQueryParameter(getSuggestionQueryParam(), query).build().toString();
    }

    




    final public String resultsUriForQuery(String query) {
        return getResultsUri().buildUpon().appendQueryParameter(getResultsQueryParam(), query).build().toString();
    }

    


    final public String getSuggestionTemplate(String placeholder) {
        if (suggestionTemplate == null) {
            suggestionTemplate = suggestUriForQuery(placeholder);
        }
        return suggestionTemplate;
    }

    




    protected abstract String getInjectableCss();

    




    protected abstract Uri getResultsUri();

    




    protected abstract Uri getSuggestionUri();

    



    protected abstract String getSuggestionQueryParam();

    



    protected abstract String getResultsQueryParam();
}

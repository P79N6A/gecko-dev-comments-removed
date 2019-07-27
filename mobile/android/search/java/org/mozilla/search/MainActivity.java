



package org.mozilla.search;

import android.content.AsyncQueryHandler;
import android.content.ContentValues;
import android.content.Intent;
import android.graphics.Rect;
import android.os.Bundle;
import android.text.TextUtils;
import android.view.View;
import android.view.animation.AccelerateDecelerateInterpolator;
import android.view.animation.Interpolator;
import android.widget.TextView;

import com.nineoldandroids.animation.Animator;
import com.nineoldandroids.animation.AnimatorSet;
import com.nineoldandroids.animation.ObjectAnimator;

import org.mozilla.gecko.LocaleAware;
import org.mozilla.gecko.Telemetry;
import org.mozilla.gecko.TelemetryContract;
import org.mozilla.gecko.db.BrowserContract.SearchHistory;
import org.mozilla.search.autocomplete.ClearableEditText;
import org.mozilla.search.autocomplete.SuggestionsFragment;
import org.mozilla.search.providers.SearchEngine;
import org.mozilla.search.providers.SearchEngineManager;
import org.mozilla.search.providers.SearchEngineManager.SearchEngineCallback;







public class MainActivity extends LocaleAware.LocaleAwareFragmentActivity
        implements AcceptsSearchQuery, SearchEngineCallback {

    private static final String KEY_SEARCH_STATE = "search_state";
    private static final String KEY_EDIT_STATE = "edit_state";
    private static final String KEY_QUERY = "query";

    static enum SearchState {
        PRESEARCH,
        POSTSEARCH
    }

    static enum EditState {
        WAITING,
        EDITING
    }

    
    private SearchState searchState = SearchState.PRESEARCH;
    private EditState editState = EditState.WAITING;

    private SearchEngineManager searchEngineManager;

    
    private SearchEngine engine;

    private SuggestionsFragment suggestionsFragment;
    private PostSearchFragment postSearchFragment;

    private AsyncQueryHandler queryHandler;

    
    private ClearableEditText editText;
    private View preSearch;
    private View postSearch;

    private View settingsButton;

    private View suggestions;

    private static final int SUGGESTION_TRANSITION_DURATION = 300;
    private static final Interpolator SUGGESTION_TRANSITION_INTERPOLATOR =
            new AccelerateDecelerateInterpolator();

    
    private TextView animationText;
    private View animationCard;

    
    private int cardPaddingX;
    private int cardPaddingY;

    
    private int textEndY;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.search_activity_main);

        suggestionsFragment = (SuggestionsFragment) getSupportFragmentManager().findFragmentById(R.id.suggestions);
        postSearchFragment = (PostSearchFragment)  getSupportFragmentManager().findFragmentById(R.id.postsearch);

        searchEngineManager = new SearchEngineManager(this);
        searchEngineManager.setChangeCallback(this);

        
        searchEngineManager.getEngine(this);

        queryHandler = new AsyncQueryHandler(getContentResolver()) {};

        editText = (ClearableEditText) findViewById(R.id.search_edit_text);
        editText.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                setEditState(EditState.EDITING);
            }
        });

        editText.setTextListener(new ClearableEditText.TextListener() {
            @Override
            public void onChange(String text) {
                
                if (editState == EditState.EDITING) {
                    suggestionsFragment.loadSuggestions(text);
                }
            }

            @Override
            public void onSubmit(String text) {
                
                final String trimmedQuery = text.trim();
                if (!TextUtils.isEmpty(trimmedQuery)) {
                    onSearch(trimmedQuery);
                }
            }

            @Override
            public void onFocusChange(boolean hasFocus) {
                setEditState(hasFocus ? EditState.EDITING : EditState.WAITING);
            }
        });

        preSearch = findViewById(R.id.presearch);
        postSearch = findViewById(R.id.postsearch);

        settingsButton = findViewById(R.id.settings_button);

        
        settingsButton.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                startActivity(new Intent(MainActivity.this, SearchPreferenceActivity.class));
            }
        });

        suggestions = findViewById(R.id.suggestions);

        animationText = (TextView) findViewById(R.id.animation_text);
        animationCard = findViewById(R.id.animation_card);

        cardPaddingX = getResources().getDimensionPixelSize(R.dimen.card_background_padding_x);
        cardPaddingY = getResources().getDimensionPixelSize(R.dimen.card_background_padding_y);
        textEndY = getResources().getDimensionPixelSize(R.dimen.animation_text_translation_y);

        if (savedInstanceState != null) {
            setSearchState(SearchState.valueOf(savedInstanceState.getString(KEY_SEARCH_STATE)));
            setEditState(EditState.valueOf(savedInstanceState.getString(KEY_EDIT_STATE)));

            final String query = savedInstanceState.getString(KEY_QUERY);
            editText.setText(query);

            
            if (searchState == SearchState.POSTSEARCH) {
                startSearch(query);
            }
        } else {
            
            
            setEditState(EditState.EDITING);
        }
    }

    @Override
    protected void onDestroy() {
        super.onDestroy();
        searchEngineManager.destroy();
        searchEngineManager = null;
        engine = null;
        suggestionsFragment = null;
        postSearchFragment = null;
        queryHandler = null;
        editText = null;
        preSearch = null;
        postSearch = null;
        settingsButton = null;
        suggestions = null;
        animationText = null;
        animationCard = null;
    }

    @Override
    protected void onStart() {
        super.onStart();
        Telemetry.startUISession(TelemetryContract.Session.SEARCH_ACTIVITY);
    }

    @Override
    protected void onStop() {
        super.onStop();
        Telemetry.stopUISession(TelemetryContract.Session.SEARCH_ACTIVITY);
    }

    @Override
    public void onNewIntent(Intent intent) {
        
        setSearchState(SearchState.PRESEARCH);

        
        
        setEditState(EditState.EDITING);
        editText.setText("");
    }

    @Override
    public void onSaveInstanceState(Bundle outState) {
        super.onSaveInstanceState(outState);

        outState.putString(KEY_SEARCH_STATE, searchState.toString());
        outState.putString(KEY_EDIT_STATE, editState.toString());
        outState.putString(KEY_QUERY, editText.getText());
    }

    @Override
    public void onSuggest(String query) {
        editText.setText(query);
    }

    @Override
    public void onSearch(String query) {
        onSearch(query, null);
    }

    @Override
    public void onSearch(String query, SuggestionAnimation suggestionAnimation) {
        storeQuery(query);

        startSearch(query);

        if (suggestionAnimation != null) {
            
            animateSuggestion(query, suggestionAnimation);
        } else {
            
            setEditState(EditState.WAITING);
            setSearchState(SearchState.POSTSEARCH);
        }
    }

    private void startSearch(final String query) {
        if (engine != null) {
            postSearchFragment.startSearch(engine, query);
            return;
        }

        
        
        searchEngineManager.getEngine(new SearchEngineCallback() {
            @Override
            public void execute(SearchEngine engine) {
                postSearchFragment.startSearch(engine, query);
            }
        });
    }

    






    @Override
    public void execute(SearchEngine engine) {
        this.engine = engine;
        suggestionsFragment.setEngine(engine);
    }

    








    private void animateSuggestion(final String query, final SuggestionAnimation suggestionAnimation) {
        animationText.setText(query);

        final Rect startBounds = suggestionAnimation.getStartBounds();
        final Rect endBounds = new Rect();
        animationCard.getGlobalVisibleRect(endBounds, null);

        
        final float cardStartY = startBounds.centerY() - endBounds.centerY();

        
        final float startScaleX = (float) (startBounds.width() - cardPaddingX * 2) / endBounds.width();
        final float startScaleY = (float) (startBounds.height() - cardPaddingY * 2) / endBounds.height();

        animationText.setVisibility(View.VISIBLE);
        animationCard.setVisibility(View.VISIBLE);

        final AnimatorSet set = new AnimatorSet();
        set.playTogether(
                ObjectAnimator.ofFloat(animationText, "translationY", startBounds.top, textEndY),
                ObjectAnimator.ofFloat(animationCard, "translationY", cardStartY, 0),
                ObjectAnimator.ofFloat(animationCard, "alpha", 0.5f, 1),
                ObjectAnimator.ofFloat(animationCard, "scaleX", startScaleX, 1f),
                ObjectAnimator.ofFloat(animationCard, "scaleY", startScaleY, 1f)
        );

        set.addListener(new Animator.AnimatorListener() {
            @Override
            public void onAnimationStart(Animator animation) {
            }

            @Override
            public void onAnimationEnd(Animator animation) {
                setEditState(EditState.WAITING);
                setSearchState(SearchState.POSTSEARCH);

                editText.setText(query);

                
                animationText.clearAnimation();
                animationCard.clearAnimation();

                animationText.setVisibility(View.INVISIBLE);
                animationCard.setVisibility(View.INVISIBLE);
            }

            @Override
            public void onAnimationCancel(Animator animation) {
            }

            @Override
            public void onAnimationRepeat(Animator animation) {
            }
        });

        set.setDuration(SUGGESTION_TRANSITION_DURATION);
        set.setInterpolator(SUGGESTION_TRANSITION_INTERPOLATOR);

        set.start();
    }

    private void setEditState(EditState editState) {
        if (this.editState == editState) {
            return;
        }
        this.editState = editState;

        updateSettingsButtonVisibility();

        editText.setActive(editState == EditState.EDITING);
        suggestions.setVisibility(editState == EditState.EDITING ? View.VISIBLE : View.INVISIBLE);
    }

    private void setSearchState(SearchState searchState) {
        if (this.searchState == searchState) {
            return;
        }
        this.searchState = searchState;

        updateSettingsButtonVisibility();

        preSearch.setVisibility(searchState == SearchState.PRESEARCH ? View.VISIBLE : View.INVISIBLE);
        postSearch.setVisibility(searchState == SearchState.POSTSEARCH ? View.VISIBLE : View.INVISIBLE);
    }

    private void updateSettingsButtonVisibility() {
        
        if (searchState == SearchState.PRESEARCH && editState == EditState.WAITING) {
            settingsButton.setVisibility(View.VISIBLE);
        } else {
            settingsButton.setVisibility(View.INVISIBLE);
        }
    }

    @Override
    public void onBackPressed() {
        if (editState == EditState.EDITING) {
            setEditState(EditState.WAITING);
        } else if (searchState == SearchState.POSTSEARCH) {
            setSearchState(SearchState.PRESEARCH);
        } else {
            super.onBackPressed();
        }
    }

    


    private void storeQuery(String query) {
        final ContentValues cv = new ContentValues();
        cv.put(SearchHistory.QUERY, query);
        
        
        queryHandler.startInsert(0, null, SearchHistory.CONTENT_URI, cv);
    }
}

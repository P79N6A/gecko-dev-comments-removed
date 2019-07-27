



package org.mozilla.search;

import android.content.AsyncQueryHandler;
import android.content.ContentValues;
import android.graphics.Rect;
import android.net.Uri;
import android.os.Bundle;
import android.support.v4.app.FragmentActivity;
import android.view.View;
import android.view.animation.AccelerateDecelerateInterpolator;
import android.view.animation.Interpolator;
import android.widget.TextView;

import com.nineoldandroids.animation.Animator;
import com.nineoldandroids.animation.AnimatorSet;
import com.nineoldandroids.animation.ObjectAnimator;

import org.mozilla.gecko.Telemetry;
import org.mozilla.gecko.TelemetryContract;
import org.mozilla.gecko.db.BrowserContract.SearchHistory;
import org.mozilla.search.autocomplete.SearchFragment;









public class MainActivity extends FragmentActivity implements AcceptsSearchQuery {

    enum State {
        START,
        PRESEARCH,
        POSTSEARCH
    }

    private State state = State.START;
    private AsyncQueryHandler queryHandler;

    private static final int SUGGESTION_TRANSITION_DURATION = 300;
    private static final Interpolator SUGGESTION_TRANSITION_INTERPOLATOR =
            new AccelerateDecelerateInterpolator();

    
    private TextView animationText;
    private View animationCard;

    
    private int cardPaddingX;
    private int cardPaddingY;

    
    private int textEndY;

    @Override
    protected void onCreate(Bundle stateBundle) {
        super.onCreate(stateBundle);
        setContentView(R.layout.search_activity_main);

        queryHandler = new AsyncQueryHandler(getContentResolver()) {};

        animationText = (TextView) findViewById(R.id.animation_text);
        animationCard = findViewById(R.id.animation_card);

        cardPaddingX = getResources().getDimensionPixelSize(R.dimen.card_background_padding_x);
        cardPaddingY = getResources().getDimensionPixelSize(R.dimen.card_background_padding_y);
        textEndY = getResources().getDimensionPixelSize(R.dimen.animation_text_translation_y);
    }

    @Override
    protected void onDestroy() {
        super.onDestroy();
        queryHandler = null;

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
    protected void onResume() {
        super.onResume();
        
        startPresearch();
    }

    @Override
    public void onSearch(String query) {
        onSearch(query, null);
    }

    @Override
    public void onSearch(String query, SuggestionAnimation suggestionAnimation) {
        storeQuery(query);

        ((PostSearchFragment) getSupportFragmentManager().findFragmentById(R.id.postsearch))
                .startSearch(query);

        if (suggestionAnimation != null) {
            
            animateSuggestion(query, suggestionAnimation);
        } else {
            
            startPostsearch();
        }
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
                
                
                suggestionAnimation.onAnimationEnd();

                
                final SearchFragment searchFragment = ((SearchFragment) getSupportFragmentManager().findFragmentById(R.id.search_fragment));
                searchFragment.setSearchTerm(query);

                startPostsearch();

                
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

    private void startPresearch() {
        if (state != State.PRESEARCH) {
            state = State.PRESEARCH;
            findViewById(R.id.postsearch).setVisibility(View.INVISIBLE);
            findViewById(R.id.presearch).setVisibility(View.VISIBLE);
        }
    }

    private void startPostsearch() {
        if (state != State.POSTSEARCH) {
            state = State.POSTSEARCH;
            findViewById(R.id.presearch).setVisibility(View.INVISIBLE);
            findViewById(R.id.postsearch).setVisibility(View.VISIBLE);
        }
    }

    @Override
    public void onBackPressed() {
        if (state == State.POSTSEARCH) {
            startPresearch();
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






package org.mozilla.gecko.tabqueue;

import org.mozilla.gecko.Locales;
import org.mozilla.gecko.R;
import org.mozilla.gecko.animation.TransitionsTracker;

import android.os.Bundle;
import android.os.Handler;
import android.view.MotionEvent;
import android.view.View;
import com.nineoldandroids.animation.Animator;
import com.nineoldandroids.animation.AnimatorListenerAdapter;
import com.nineoldandroids.animation.AnimatorSet;
import com.nineoldandroids.animation.ObjectAnimator;
import com.nineoldandroids.view.ViewHelper;

public class TabQueuePrompt extends Locales.LocaleAwareActivity {
    public static final String LOGTAG = "Gecko" + TabQueuePrompt.class.getSimpleName();

    
    private boolean isAnimating;

    private View containerView;
    private View buttonContainer;
    private View enabledConfirmation;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);

        showTabQueueEnablePrompt();
    }

    private void showTabQueueEnablePrompt() {
        setContentView(R.layout.tab_queue_prompt);

        findViewById(R.id.ok_button).setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                onConfirmButtonPressed();
            }
        });
        findViewById(R.id.cancel_button).setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                setResult(TabQueueHelper.TAB_QUEUE_NO);
                finish();
            }
        });

        containerView = findViewById(R.id.tab_queue_container);
        buttonContainer = findViewById(R.id.button_container);
        enabledConfirmation = findViewById(R.id.enabled_confirmation);

        ViewHelper.setTranslationY(containerView, 500);
        ViewHelper.setAlpha(containerView, 0);

        final Animator translateAnimator = ObjectAnimator.ofFloat(containerView, "translationY", 0);
        translateAnimator.setDuration(400);

        final Animator alphaAnimator = ObjectAnimator.ofFloat(containerView, "alpha", 1);
        alphaAnimator.setStartDelay(200);
        alphaAnimator.setDuration(600);

        final AnimatorSet set = new AnimatorSet();
        set.playTogether(alphaAnimator, translateAnimator);
        set.setStartDelay(400);
        TransitionsTracker.track(set);

        set.start();
    }

    @Override
    public void finish() {
        super.finish();

        
        overridePendingTransition(0, 0);
    }

    private void onConfirmButtonPressed() {
        enabledConfirmation.setVisibility(View.VISIBLE);
        ViewHelper.setAlpha(enabledConfirmation, 0);

        final Animator buttonsAlphaAnimator = ObjectAnimator.ofFloat(buttonContainer, "alpha", 0);
        buttonsAlphaAnimator.setDuration(300);

        final Animator messagesAlphaAnimator = ObjectAnimator.ofFloat(enabledConfirmation, "alpha", 1);
        messagesAlphaAnimator.setDuration(300);
        messagesAlphaAnimator.setStartDelay(200);

        final AnimatorSet set = new AnimatorSet();
        set.playTogether(buttonsAlphaAnimator, messagesAlphaAnimator);
        TransitionsTracker.track(set);

        set.addListener(new AnimatorListenerAdapter() {

            @Override
            public void onAnimationEnd(Animator animation) {

                new Handler().postDelayed(new Runnable() {
                    @Override
                    public void run() {
                        slideOut();
                        setResult(TabQueueHelper.TAB_QUEUE_YES);
                    }
                }, 1000);
            }
        });

        set.start();
    }

    


    private void slideOut() {
        if (isAnimating) {
            return;
        }

        isAnimating = true;

        ObjectAnimator animator = ObjectAnimator.ofFloat(containerView, "translationY", containerView.getHeight());
        animator.addListener(new AnimatorListenerAdapter() {

            @Override
            public void onAnimationEnd(Animator animation) {
                finish();
            }

        });
        animator.start();
    }

    


    @Override
    public void onBackPressed() {
        slideOut();
    }

    


    @Override
    public boolean onTouchEvent(MotionEvent event) {
        slideOut();
        return true;
    }
}





package org.mozilla.gecko.trackingprotection;

import org.mozilla.gecko.Locales;
import org.mozilla.gecko.R;
import org.mozilla.gecko.animation.TransitionsTracker;
import org.mozilla.gecko.preferences.GeckoPreferences;
import org.mozilla.gecko.util.HardwareUtils;

import android.content.Intent;
import android.os.Bundle;
import android.view.MotionEvent;
import android.view.View;
import com.nineoldandroids.animation.Animator;
import com.nineoldandroids.animation.AnimatorListenerAdapter;
import com.nineoldandroids.animation.AnimatorSet;
import com.nineoldandroids.animation.ObjectAnimator;
import com.nineoldandroids.view.ViewHelper;

public class TrackingProtectionPrompt extends Locales.LocaleAwareActivity {
        public static final String LOGTAG = "Gecko" + TrackingProtectionPrompt.class.getSimpleName();

        
        private boolean isAnimating;

        private View containerView;

        @Override
        protected void onCreate(Bundle savedInstanceState) {
            super.onCreate(savedInstanceState);

            showPrompt();
        }

        private void showPrompt() {
            setContentView(R.layout.tracking_protection_prompt);

            findViewById(R.id.ok_button).setOnClickListener(new View.OnClickListener() {
                @Override
                public void onClick(View v) {
                    onConfirmButtonPressed();
                }
            });
            findViewById(R.id.link_text).setOnClickListener(new View.OnClickListener() {
                @Override
                public void onClick(View v) {
                    slideOut();
                    final Intent settingsIntent = new Intent(TrackingProtectionPrompt.this, GeckoPreferences.class);
                    GeckoPreferences.setResourceToOpen(settingsIntent, "preferences_privacy");
                    startActivity(settingsIntent);

                    
                    
                    if (HardwareUtils.IS_KINDLE_DEVICE) {
                        overridePendingTransition(0, 0);
                    }
                }
            });

            containerView = findViewById(R.id.tracking_protection_inner_container);

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
            slideOut();
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

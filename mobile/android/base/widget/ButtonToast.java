















package org.mozilla.gecko.widget;

import java.util.LinkedList;

import org.mozilla.gecko.R;
import org.mozilla.gecko.animation.PropertyAnimator;

import android.graphics.drawable.Drawable;
import android.os.Bundle;
import android.os.Handler;
import android.view.View;
import android.widget.Button;
import android.widget.TextView;

public class ButtonToast {
    @SuppressWarnings("unused")
    private final static String LOGTAG = "GeckoButtonToast";

    private final static int TOAST_DURATION = 5000;

    private final View mView;
    private final TextView mMessageView;
    private final Button mButton;
    private final Handler mHideHandler = new Handler();

    private final LinkedList<Toast> mQueue = new LinkedList<Toast>();
    private Toast mCurrentToast;

    public enum ReasonHidden {
        CLICKED,
        TIMEOUT,
        STARTUP
    }

    
    private static class Toast {
        public final CharSequence buttonMessage;
        public Drawable buttonDrawable;
        public final CharSequence message;
        public ToastListener listener;

        public Toast(CharSequence aMessage, CharSequence aButtonMessage,
                     Drawable aDrawable, ToastListener aListener) {
            message = aMessage;
            buttonMessage = aButtonMessage;
            buttonDrawable = aDrawable;
            listener = aListener;
        }
    }

    public interface ToastListener {
        void onButtonClicked();
        void onToastHidden(ReasonHidden reason);
    }

    public ButtonToast(View view) {
        mView = view;
        mMessageView = (TextView) mView.findViewById(R.id.toast_message);
        mButton = (Button) mView.findViewById(R.id.toast_button);
        mButton.setOnClickListener(new View.OnClickListener() {
                    @Override
                    public void onClick(View view) {
                        Toast t = mCurrentToast;
                        if (t == null)
                            return;

                        hide(false, ReasonHidden.CLICKED);
                        if (t.listener != null) {
                            t.listener.onButtonClicked();
                        }
                    }
                });

        hide(true, ReasonHidden.STARTUP);
    }

    public void show(boolean immediate, CharSequence message,
                     CharSequence buttonMessage, int buttonDrawableId,
                     ToastListener listener) {
        final Drawable d = mView.getContext().getResources().getDrawable(buttonDrawableId);
        show(false, message, buttonMessage, d, listener);
    }

    public void show(boolean immediate, CharSequence message,
                     CharSequence buttonMessage, Drawable buttonDrawable,
                     ToastListener listener) {
        show(new Toast(message, buttonMessage, buttonDrawable, listener), immediate);
    }

    private void show(Toast t, boolean immediate) {
        
        if (mView.getVisibility() == View.VISIBLE) {
            mQueue.offer(t);
            return;
        }

        mCurrentToast = t;
        mButton.setEnabled(true);

        
        mMessageView.setText(null != t.message ? t.message : "");
        mButton.setText(null != t.buttonMessage ? t.buttonMessage : "");
        if (null != t.buttonDrawable) {
            mButton.setCompoundDrawablePadding(mView.getContext().getResources().getDimensionPixelSize(R.dimen.toast_button_padding));
            mButton.setCompoundDrawablesWithIntrinsicBounds(t.buttonDrawable, null, null, null);
        } else {
            mButton.setCompoundDrawablePadding(0);
            mButton.setCompoundDrawablesWithIntrinsicBounds(null, null, null, null);
        }

        mHideHandler.removeCallbacks(mHideRunnable);
        mHideHandler.postDelayed(mHideRunnable, TOAST_DURATION);

        mView.setVisibility(View.VISIBLE);
        int duration = immediate ? 0 : mView.getResources().getInteger(android.R.integer.config_longAnimTime);

        PropertyAnimator animator = new PropertyAnimator(duration);
        animator.attach(mView, PropertyAnimator.Property.ALPHA, 1.0f);
        animator.start();
    }

    public void hide(boolean immediate, ReasonHidden reason) {
        if (mButton.isPressed() && reason != ReasonHidden.CLICKED) {
            mHideHandler.postDelayed(mHideRunnable, TOAST_DURATION);
            return;
        }

        if (mCurrentToast != null && mCurrentToast.listener != null) {
            mCurrentToast.listener.onToastHidden(reason);
        }
        mCurrentToast = null;
        mButton.setEnabled(false);
        mHideHandler.removeCallbacks(mHideRunnable);
        int duration = immediate ? 0 : mView.getResources().getInteger(android.R.integer.config_longAnimTime);

        mView.clearAnimation();
        if (immediate) {
            mView.setVisibility(View.GONE);
            showNextInQueue();
        } else {
            
            
            PropertyAnimator animator = new PropertyAnimator(duration);
            animator.attach(mView, PropertyAnimator.Property.ALPHA, 0.0f);
            animator.addPropertyAnimationListener(new PropertyAnimator.PropertyAnimationListener () {
                
                
                public void onPropertyAnimationEnd() {
                    mView.setVisibility(View.GONE);
                    showNextInQueue();
                }
                public void onPropertyAnimationStart() { }
            });
            animator.start();
        }
    }

    public void onSaveInstanceState(Bundle outState) {
        
        if (mCurrentToast != null) {
            mQueue.add(0, mCurrentToast);
        }
    }

    private void showNextInQueue() {
        Toast t = mQueue.poll();
        if (t != null) {
            show(t, false);
        }
    }

    private Runnable mHideRunnable = new Runnable() {
        @Override
        public void run() {
            hide(false, ReasonHidden.TIMEOUT);
        }
    };
}

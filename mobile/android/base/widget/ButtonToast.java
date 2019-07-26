















package org.mozilla.gecko.widget;

import android.animation.Animator;
import android.animation.AnimatorListenerAdapter;
import android.graphics.drawable.BitmapDrawable;
import android.graphics.drawable.Drawable;
import android.os.Bundle;
import android.os.Handler;
import android.text.TextUtils;
import android.view.animation.Animation;
import android.view.animation.AlphaAnimation;
import android.view.View;
import android.widget.Button;
import android.widget.TextView;

import java.util.LinkedList;

import org.mozilla.gecko.R;
import org.mozilla.gecko.gfx.BitmapUtils;

public class ButtonToast {
    private final static String LOGTAG = "GeckoButtonToast";
    private final static int TOAST_DURATION = 5000;

    private final View mView;
    private final TextView mMessageView;
    private final Button mButton;
    private final Handler mHideHandler = new Handler();

    private final ToastListener mListener;
    private final LinkedList<Toast> mQueue = new LinkedList<Toast>();
    private Toast mCurrentToast;

    
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
    }

    public ButtonToast(View view) {
        mView = view;
        mListener = null;
        mMessageView = (TextView) mView.findViewById(R.id.toast_message);
        mButton = (Button) mView.findViewById(R.id.toast_button);
        mButton.setOnClickListener(new View.OnClickListener() {
                    @Override
                    public void onClick(View view) {
                        Toast t = mCurrentToast;
                        if (t == null)
                            return;

                        hide(false);
                        if (t.listener != null) {
                            t.listener.onButtonClicked();
                        }
                    }
                });

        hide(true);
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

        mMessageView.setText(t.message);
        mButton.setText(t.buttonMessage);
        mButton.setCompoundDrawablePadding(mView.getContext().getResources().getDimensionPixelSize(R.dimen.toast_button_padding));
        mButton.setCompoundDrawablesWithIntrinsicBounds(null, null, t.buttonDrawable, null);

        mHideHandler.removeCallbacks(mHideRunnable);
        mHideHandler.postDelayed(mHideRunnable, TOAST_DURATION);

        mView.setVisibility(View.VISIBLE);
        int duration = immediate ? 0 : mView.getResources().getInteger(android.R.integer.config_longAnimTime);

        mView.clearAnimation();
        AlphaAnimation alpha = new AlphaAnimation(0.0f, 1.0f);
        alpha.setDuration(duration);
        alpha.setFillAfter(true);
        mView.startAnimation(alpha);
    }

    public void hide(boolean immediate) {
        if (mButton.isPressed()) {
            mHideHandler.postDelayed(mHideRunnable, TOAST_DURATION);
            return;
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
            AlphaAnimation alpha = new AlphaAnimation(1.0f, 0.0f);
            alpha.setDuration(duration);
            alpha.setFillAfter(true);
            alpha.setAnimationListener(new Animation.AnimationListener () {
                
                
                public void onAnimationEnd(Animation animation) {
                    mView.setVisibility(View.GONE);
                    showNextInQueue();
                }
                public void onAnimationRepeat(Animation animation) { }
                public void onAnimationStart(Animation animation) { }
            });
            mView.startAnimation(alpha);
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
            hide(false);
        }
    };
}

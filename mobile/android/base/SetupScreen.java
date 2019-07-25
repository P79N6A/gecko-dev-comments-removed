




































package org.mozilla.gecko;

import android.app.Dialog;
import android.content.Context;
import android.graphics.drawable.AnimationDrawable;
import android.graphics.drawable.Drawable;
import android.os.Bundle;
import android.os.Handler;
import android.util.Log;
import android.widget.ImageView;
import java.util.Timer;
import java.util.TimerTask;

import org.mozilla.gecko.R;

public class SetupScreen extends Dialog
{
    private static final String LOGTAG = "SetupScreen";
    
    private static final int DEFAULT_DELAY = 100;
    private AnimationDrawable mProgressSpinner;
    private Context mContext;
    private Timer mTimer;
    private TimerTask mShowTask;

    public class ShowTask extends TimerTask {
        private Handler mHandler;

        public ShowTask(Handler aHandler) {
            mHandler = aHandler;
        }

        @Override
        public void run() {
            mHandler.post(new Runnable() {
                    public void run() {
                        SetupScreen.this.show();
                    }
            });

        }

        @Override
        public boolean cancel() {
            boolean stillInQueue = super.cancel();
            if (!stillInQueue) {
                mHandler.post(new Runnable() {
                        public void run() {
                            
                            
                            SetupScreen.super.dismiss();
                        }
                });
            }
            return stillInQueue;
        }
    }

    public SetupScreen(Context aContext) {
        super(aContext, android.R.style.Theme_NoTitleBar_Fullscreen);
        mContext = aContext;
    }

    @Override
    public void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.setup_screen);
        setCancelable(false);

        setTitle(R.string.splash_firstrun);

        ImageView spinnerImage = (ImageView)findViewById(R.id.spinner_image);
        mProgressSpinner = (AnimationDrawable)spinnerImage.getBackground();
        spinnerImage.setImageDrawable(mProgressSpinner);
    }

    @Override
    public void onWindowFocusChanged (boolean hasFocus) {
        mProgressSpinner.start();
    }

    public void showDelayed(Handler aHandler) {
        showDelayed(aHandler, DEFAULT_DELAY);
    }

    
    
    
    public void showDelayed(Handler aHandler, int aDelay) {
        mTimer = new Timer("SetupScreen");
        mShowTask = new ShowTask(aHandler);
        mTimer.schedule(mShowTask, aDelay);
    }

    @Override
    public void dismiss() {
        
        
        if (mShowTask != null) {
            mShowTask.cancel();
            mShowTask = null;
        } else {
            
            super.dismiss();
        }
        if (mTimer != null) {
            mTimer.cancel();
            mTimer = null;
        }
    }
}

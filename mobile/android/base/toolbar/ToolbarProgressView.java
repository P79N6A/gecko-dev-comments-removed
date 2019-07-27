















package org.mozilla.gecko.toolbar;

import org.mozilla.gecko.AppConstants.Versions;

import android.content.Context;
import android.graphics.Canvas;
import android.graphics.Rect;
import android.graphics.drawable.Drawable;
import android.os.Handler;
import android.os.Message;
import android.util.AttributeSet;
import android.view.View;
import android.view.animation.Animation;
import android.widget.ImageView;








public class ToolbarProgressView extends ImageView {
    private static final int MAX_PROGRESS = 10000;
    private static final int MSG_UPDATE = 0;
    private static final int MSG_HIDE = 1;
    private static final int STEPS = 10;
    private static final int DELAY = 40;

    private int mTargetProgress;
    private int mIncrement;
    private Rect mBounds;
    private Handler mHandler;
    private int mCurrentProgress;

    public ToolbarProgressView(Context context, AttributeSet attrs, int defStyle) {
        super(context, attrs, defStyle);
        init(context);
    }

    public ToolbarProgressView(Context context, AttributeSet attrs) {
        super(context, attrs);
        init(context);
    }

    public ToolbarProgressView(Context context) {
        super(context);
        init(context);
    }

    private void init(Context ctx) {
        mBounds = new Rect(0,0,0,0);
        mTargetProgress = 0;

        mHandler = new Handler() {
            @Override
            public void handleMessage(Message msg) {
                switch (msg.what) {
                    case MSG_UPDATE:
                        mCurrentProgress = Math.min(mTargetProgress, mCurrentProgress + mIncrement);

                        updateBounds();

                        if (mCurrentProgress < mTargetProgress) {
                            final int delay = (mTargetProgress < MAX_PROGRESS) ? DELAY : DELAY / 4;
                            sendMessageDelayed(mHandler.obtainMessage(msg.what), delay);
                        } else if (mCurrentProgress == MAX_PROGRESS) {
                            sendMessageDelayed(mHandler.obtainMessage(MSG_HIDE), DELAY);
                        }
                        break;

                    case MSG_HIDE:
                        setVisibility(View.GONE);
                        break;
                }
            }

        };
    }

    @Override
    public void setVisibility(int visibility) {
        
        
        if (Versions.preHC && visibility != VISIBLE) {
            clearAnimation();
        }

        super.setVisibility(visibility);
    }

    @Override
    public void setAnimation(Animation animation) {
        
        
        
        if (Versions.preHC && isShown()) {
            super.setAnimation(animation);
        }
    }

    @Override
    public void onLayout(boolean f, int l, int t, int r, int b) {
        mBounds.left = 0;
        mBounds.right = (r - l) * mCurrentProgress / MAX_PROGRESS;
        mBounds.top = 0;
        mBounds.bottom = b - t;
    }

    @Override
    public void onDraw(Canvas canvas) {
        final Drawable d = getDrawable();
        d.setBounds(mBounds);
        d.draw(canvas);
    }

    




    void setProgress(int progressPercentage) {
        mCurrentProgress = mTargetProgress = getAbsoluteProgress(progressPercentage);
        updateBounds();

        clearMessages();
    }

    





    void animateProgress(int progressPercentage) {
        final int absoluteProgress = getAbsoluteProgress(progressPercentage);
        if (absoluteProgress <= mTargetProgress) {
            
            
            
            
            return;
        }

        mTargetProgress = absoluteProgress;
        mIncrement = (mTargetProgress - mCurrentProgress) / STEPS;

        clearMessages();
        mHandler.sendEmptyMessage(MSG_UPDATE);
    }

    private void clearMessages() {
        mHandler.removeMessages(MSG_UPDATE);
        mHandler.removeMessages(MSG_HIDE);
    }

    private int getAbsoluteProgress(int progressPercentage) {
        if (progressPercentage < 0) {
            return 0;
        }

        if (progressPercentage > 100) {
            return 100;
        }

        return progressPercentage * MAX_PROGRESS / 100;
    }

    private void updateBounds() {
        mBounds.right = getWidth() * mCurrentProgress / MAX_PROGRESS;
        invalidate();
    }
}

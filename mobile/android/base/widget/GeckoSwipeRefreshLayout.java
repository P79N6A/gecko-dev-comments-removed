















package org.mozilla.gecko.widget;

import org.mozilla.gecko.AppConstants.Versions;

import android.content.Context;
import android.content.res.Resources;
import android.content.res.TypedArray;
import android.graphics.Canvas;
import android.graphics.Paint;
import android.graphics.Rect;
import android.graphics.RectF;
import android.support.v4.view.ViewCompat;
import android.util.AttributeSet;
import android.util.DisplayMetrics;
import android.view.MotionEvent;
import android.view.View;
import android.view.ViewConfiguration;
import android.view.ViewGroup;
import android.view.animation.AccelerateInterpolator;
import android.view.animation.Animation;
import android.view.animation.Animation.AnimationListener;
import android.view.animation.AnimationUtils;
import android.view.animation.DecelerateInterpolator;
import android.view.animation.Interpolator;
import android.view.animation.Transformation;
import android.widget.AbsListView;














public class GeckoSwipeRefreshLayout extends ViewGroup {
    private static final long RETURN_TO_ORIGINAL_POSITION_TIMEOUT = 300;
    private static final float ACCELERATE_INTERPOLATION_FACTOR = 1.5f;
    private static final float DECELERATE_INTERPOLATION_FACTOR = 2f;
    
    private static final float PROGRESS_BAR_HEIGHT = 3;
    private static final float MAX_SWIPE_DISTANCE_FACTOR = .6f;
    private static final int REFRESH_TRIGGER_DISTANCE = 120;

    SwipeProgressBar mProgressBar; 
    View mTarget; 
    int mOriginalOffsetTop;
    int mFrom;
    int mMediumAnimationDuration;
    float mFromPercentage;
    float mCurrPercentage;
    int mCurrentTargetOffsetTop;
    private OnRefreshListener mListener;
    private MotionEvent mDownEvent;
    private boolean mRefreshing;
    private int mTouchSlop;
    private float mDistanceToTriggerSync = -1;
    private float mPrevY;
    private int mProgressBarHeight;

    
    
    boolean mReturningToStart;
    final DecelerateInterpolator mDecelerateInterpolator;
    private final AccelerateInterpolator mAccelerateInterpolator;
    private static final int[] LAYOUT_ATTRS = new int[] {
        android.R.attr.enabled
    };

    private final Animation mAnimateToStartPosition = new Animation() {
        @Override
        public void applyTransformation(float interpolatedTime, Transformation t) {
            int targetTop = 0;
            if (mFrom != mOriginalOffsetTop) {
                targetTop = (mFrom + (int)((mOriginalOffsetTop - mFrom) * interpolatedTime));
            }
            int offset = targetTop - mTarget.getTop();
            final int currentTop = mTarget.getTop();
            if (offset + currentTop < 0) {
                offset = 0 - currentTop;
            }
            setTargetOffsetTopAndBottom(offset);
        }
    };

    Animation mShrinkTrigger = new Animation() {
        @Override
        public void applyTransformation(float interpolatedTime, Transformation t) {
            float percent = mFromPercentage + ((0 - mFromPercentage) * interpolatedTime);
            mProgressBar.setTriggerPercentage(percent);
        }
    };

    final AnimationListener mReturnToStartPositionListener = new BaseAnimationListener() {
        @Override
        public void onAnimationEnd(Animation animation) {
            
            
            mCurrentTargetOffsetTop = 0;
        }
    };

    final AnimationListener mShrinkAnimationListener = new BaseAnimationListener() {
        @Override
        public void onAnimationEnd(Animation animation) {
            mCurrPercentage = 0;
        }
    };

    private final Runnable mReturnToStartPosition = new Runnable() {

        @Override
        public void run() {
            mReturningToStart = true;
            animateOffsetToStartPosition(mCurrentTargetOffsetTop + getPaddingTop(),
                    mReturnToStartPositionListener);
        }

    };

    
    private final Runnable mCancel = new Runnable() {

        @Override
        public void run() {
            mReturningToStart = true;
            
            
            if (mProgressBar != null) {
                mFromPercentage = mCurrPercentage;
                mShrinkTrigger.setDuration(mMediumAnimationDuration);
                mShrinkTrigger.setAnimationListener(mShrinkAnimationListener);
                mShrinkTrigger.reset();
                mShrinkTrigger.setInterpolator(mDecelerateInterpolator);
                startAnimation(mShrinkTrigger);
            }
            animateOffsetToStartPosition(mCurrentTargetOffsetTop + getPaddingTop(),
                    mReturnToStartPositionListener);
        }

    };

    



    public GeckoSwipeRefreshLayout(Context context) {
        this(context, null);
    }

    




    public GeckoSwipeRefreshLayout(Context context, AttributeSet attrs) {
        super(context, attrs);

        mTouchSlop = ViewConfiguration.get(context).getScaledTouchSlop();

        mMediumAnimationDuration = getResources().getInteger(
                android.R.integer.config_mediumAnimTime);

        setWillNotDraw(false);
        mProgressBar = new SwipeProgressBar(this);
        final DisplayMetrics metrics = getResources().getDisplayMetrics();
        mProgressBarHeight = (int) (metrics.density * PROGRESS_BAR_HEIGHT);
        mDecelerateInterpolator = new DecelerateInterpolator(DECELERATE_INTERPOLATION_FACTOR);
        mAccelerateInterpolator = new AccelerateInterpolator(ACCELERATE_INTERPOLATION_FACTOR);

        final TypedArray a = context.obtainStyledAttributes(attrs, LAYOUT_ATTRS);
        setEnabled(a.getBoolean(0, true));
        a.recycle();
    }

    @Override
    public void onAttachedToWindow() {
        super.onAttachedToWindow();
        removeCallbacks(mCancel);
        removeCallbacks(mReturnToStartPosition);

        
        
        if (getChildCount() > 0) {
            getChildAt(0).forceLayout();
        }
    }

    @Override
    public void onDetachedFromWindow() {
        super.onDetachedFromWindow();
        removeCallbacks(mReturnToStartPosition);
        removeCallbacks(mCancel);
    }

    void animateOffsetToStartPosition(int from, AnimationListener listener) {
        mFrom = from;
        mAnimateToStartPosition.reset();
        mAnimateToStartPosition.setDuration(mMediumAnimationDuration);
        mAnimateToStartPosition.setAnimationListener(listener);
        mAnimateToStartPosition.setInterpolator(mDecelerateInterpolator);
        mTarget.startAnimation(mAnimateToStartPosition);
    }

    



    public void setOnRefreshListener(OnRefreshListener listener) {
        mListener = listener;
    }

    private void setTriggerPercentage(float percent) {
        if (percent == 0f) {
            
            
            mCurrPercentage = 0;
            return;
        }
        mCurrPercentage = percent;
        mProgressBar.setTriggerPercentage(percent);
    }

    





    public void setRefreshing(boolean refreshing) {
        if (mRefreshing != refreshing) {
            ensureTarget();
            mCurrPercentage = 0;
            mRefreshing = refreshing;
            if (mRefreshing) {
                mProgressBar.start();
            } else {
                mProgressBar.stop();
            }
        }
    }

    









    public void setColorScheme(int colorRes1, int colorRes2, int colorRes3, int colorRes4) {
        ensureTarget();
        final Resources res = getResources();
        final int color1 = res.getColor(colorRes1);
        final int color2 = res.getColor(colorRes2);
        final int color3 = res.getColor(colorRes3);
        final int color4 = res.getColor(colorRes4);
        mProgressBar.setColorScheme(color1, color2, color3,color4);
    }

    



    public boolean isRefreshing() {
        return mRefreshing;
    }

    private void ensureTarget() {
        
        if (mTarget == null) {
            if (getChildCount() > 1 && !isInEditMode()) {
                throw new IllegalStateException(
                        "GeckoSwipeRefreshLayout can host only one direct child");
            }
            mTarget = getChildAt(0);
            mOriginalOffsetTop = mTarget.getTop() + getPaddingTop();
        }
        if (mDistanceToTriggerSync == -1) {
            if (getParent() != null && ((View)getParent()).getHeight() > 0) {
                final DisplayMetrics metrics = getResources().getDisplayMetrics();
                mDistanceToTriggerSync = (int) Math.min(
                        ((View) getParent()) .getHeight() * MAX_SWIPE_DISTANCE_FACTOR,
                                REFRESH_TRIGGER_DISTANCE * metrics.density);
            }
        }
    }

    @Override
    public void draw(Canvas canvas) {
        super.draw(canvas);
        mProgressBar.draw(canvas);
    }

    @Override
    protected void onLayout(boolean changed, int left, int top, int right, int bottom) {
        final int width =  getMeasuredWidth();
        final int height = getMeasuredHeight();
        mProgressBar.setBounds(0, 0, width, mProgressBarHeight);
        if (getChildCount() == 0) {
            return;
        }
        final View child = getChildAt(0);
        final int childLeft = getPaddingLeft();
        final int childTop = mCurrentTargetOffsetTop + getPaddingTop();
        final int childWidth = width - getPaddingLeft() - getPaddingRight();
        final int childHeight = height - getPaddingTop() - getPaddingBottom();
        child.layout(childLeft, childTop, childLeft + childWidth, childTop + childHeight);
    }

    @Override
    public void onMeasure(int widthMeasureSpec, int heightMeasureSpec) {
        super.onMeasure(widthMeasureSpec, heightMeasureSpec);
        if (getChildCount() > 1 && !isInEditMode()) {
            throw new IllegalStateException("GeckoSwipeRefreshLayout can host only one direct child");
        }
        if (getChildCount() > 0) {
            getChildAt(0).measure(
                    MeasureSpec.makeMeasureSpec(
                            getMeasuredWidth() - getPaddingLeft() - getPaddingRight(),
                            MeasureSpec.EXACTLY),
                    MeasureSpec.makeMeasureSpec(
                            getMeasuredHeight() - getPaddingTop() - getPaddingBottom(),
                            MeasureSpec.EXACTLY));
        }
    }

    



    public boolean canChildScrollUp() {
        if (Versions.preICS) {
            if (mTarget instanceof AbsListView) {
                final AbsListView absListView = (AbsListView) mTarget;
                return absListView.getChildCount() > 0
                        && (absListView.getFirstVisiblePosition() > 0 || absListView.getChildAt(0)
                                .getTop() < absListView.getPaddingTop());
            } else {
                return mTarget.getScrollY() > 0;
            }
        } else {
            return ViewCompat.canScrollVertically(mTarget, -1);
        }
    }

    @Override
    public boolean onInterceptTouchEvent(MotionEvent ev) {
        ensureTarget();
        boolean handled = false;
        if (mReturningToStart && ev.getAction() == MotionEvent.ACTION_DOWN) {
            mReturningToStart = false;
        }
        if (isEnabled() && !mReturningToStart && !canChildScrollUp()) {
            handled = onTouchEvent(ev);
        }
        return !handled ? super.onInterceptTouchEvent(ev) : handled;
    }

    @Override
    public void requestDisallowInterceptTouchEvent(boolean b) {
        
    }

    @Override
    public boolean onTouchEvent(MotionEvent event) {
        final int action = event.getAction();
        boolean handled = false;
        switch (action) {
            case MotionEvent.ACTION_DOWN:
                mCurrPercentage = 0;
                mDownEvent = MotionEvent.obtain(event);
                mPrevY = mDownEvent.getY();
                break;
            case MotionEvent.ACTION_MOVE:
                if (mDownEvent != null && !mReturningToStart) {
                    final float eventY = event.getY();
                    float yDiff = eventY - mDownEvent.getY();
                    if (yDiff > mTouchSlop) {
                        
                        if (yDiff > mDistanceToTriggerSync) {
                            
                            startRefresh();
                            handled = true;
                            break;
                        } else {
                            
                            setTriggerPercentage(
                                    mAccelerateInterpolator.getInterpolation(
                                            yDiff / mDistanceToTriggerSync));
                            float offsetTop = yDiff;
                            if (mPrevY > eventY) {
                                offsetTop = yDiff - mTouchSlop;
                            }
                            
                            
                            if (mPrevY > eventY && (mTarget.getTop() < mTouchSlop)) {
                                
                                
                                
                                removeCallbacks(mCancel);
                            } else {
                                updatePositionTimeout();
                            }
                            mPrevY = event.getY();
                            handled = true;
                        }
                    }
                }
                break;
            case MotionEvent.ACTION_UP:
            case MotionEvent.ACTION_CANCEL:
                if (mDownEvent != null) {
                    mDownEvent.recycle();
                    mDownEvent = null;
                }
                break;
        }
        return handled;
    }

    private void startRefresh() {
        removeCallbacks(mCancel);
        mReturnToStartPosition.run();
        setRefreshing(true);
        mListener.onRefresh();
    }

    private void updateContentOffsetTop(int targetTop) {
        final int currentTop = mTarget.getTop();
        if (targetTop > mDistanceToTriggerSync) {
            targetTop = (int) mDistanceToTriggerSync;
        } else if (targetTop < 0) {
            targetTop = 0;
        }
        setTargetOffsetTopAndBottom(targetTop - currentTop);
    }

    void setTargetOffsetTopAndBottom(int offset) {
        mTarget.offsetTopAndBottom(offset);
        mCurrentTargetOffsetTop = mTarget.getTop();
    }

    private void updatePositionTimeout() {
        removeCallbacks(mCancel);
        postDelayed(mCancel, RETURN_TO_ORIGINAL_POSITION_TIMEOUT);
    }

    



    public interface OnRefreshListener {
        public void onRefresh();
    }

    



    private class BaseAnimationListener implements AnimationListener {
        @Override
        public void onAnimationStart(Animation animation) {
        }

        @Override
        public void onAnimationEnd(Animation animation) {
        }

        @Override
        public void onAnimationRepeat(Animation animation) {
        }
    }

    



    private static final class SwipeProgressBar {
        
        private final static int COLOR1 = 0xB3000000;
        private final static int COLOR2 = 0x80000000;
        private final static int COLOR3 = 0x4d000000;
        private final static int COLOR4 = 0x1a000000;

        
        private static final int ANIMATION_DURATION_MS = 2000;

        
        private static final int FINISH_ANIMATION_DURATION_MS = 1000;

        
        private static final Interpolator INTERPOLATOR = BakedBezierInterpolator.getInstance();

        private final Paint mPaint = new Paint();
        private final RectF mClipRect = new RectF();
        private float mTriggerPercentage;
        private long mStartTime;
        private long mFinishTime;
        private boolean mRunning;

        
        private int mColor1;
        private int mColor2;
        private int mColor3;
        private int mColor4;
        private View mParent;

        private Rect mBounds = new Rect();

        public SwipeProgressBar(View parent) {
            mParent = parent;
            mColor1 = COLOR1;
            mColor2 = COLOR2;
            mColor3 = COLOR3;
            mColor4 = COLOR4;
        }

        









        void setColorScheme(int color1, int color2, int color3, int color4) {
            mColor1 = color1;
            mColor2 = color2;
            mColor3 = color3;
            mColor4 = color4;
        }

        




        void setTriggerPercentage(float triggerPercentage) {
            mTriggerPercentage = triggerPercentage;
            mStartTime = 0;
            ViewCompat.postInvalidateOnAnimation(mParent);
        }

        


        void start() {
            if (!mRunning) {
                mTriggerPercentage = 0;
                mStartTime = AnimationUtils.currentAnimationTimeMillis();
                mRunning = true;
                mParent.postInvalidate();
            }
        }

        


        void stop() {
            if (mRunning) {
                mTriggerPercentage = 0;
                mFinishTime = AnimationUtils.currentAnimationTimeMillis();
                mRunning = false;
                mParent.postInvalidate();
            }
        }

        


        boolean isRunning() {
            return mRunning || mFinishTime > 0;
        }

        void draw(Canvas canvas) {
            final int width = mBounds.width();
            final int height = mBounds.height();
            final int cx = width / 2;
            final int cy = height / 2;
            boolean drawTriggerWhileFinishing = false;
            int restoreCount = canvas.save();
            canvas.clipRect(mBounds);

            if (mRunning || (mFinishTime > 0)) {
                long now = AnimationUtils.currentAnimationTimeMillis();
                long elapsed = (now - mStartTime) % ANIMATION_DURATION_MS;
                long iterations = (now - mStartTime) / ANIMATION_DURATION_MS;
                float rawProgress = (elapsed / (ANIMATION_DURATION_MS / 100f));

                
                
                if (!mRunning) {
                    
                    
                    if ((now - mFinishTime) >= FINISH_ANIMATION_DURATION_MS) {
                        mFinishTime = 0;
                        return;
                    }

                    
                    
                    
                    long finishElapsed = (now - mFinishTime) % FINISH_ANIMATION_DURATION_MS;
                    float finishProgress = (finishElapsed / (FINISH_ANIMATION_DURATION_MS / 100f));
                    float pct = (finishProgress / 100f);
                    
                    float clearRadius = width / 2 * INTERPOLATOR.getInterpolation(pct);
                    mClipRect.set(cx - clearRadius, 0, cx + clearRadius, height);
                    canvas.saveLayerAlpha(mClipRect, 0, 0);
                    
                    
                    
                    
                    drawTriggerWhileFinishing = true;
                }

                
                if (iterations == 0) {
                    canvas.drawColor(mColor1);
                } else {
                    if (rawProgress >= 0 && rawProgress < 25) {
                        canvas.drawColor(mColor4);
                    } else if (rawProgress >= 25 && rawProgress < 50) {
                        canvas.drawColor(mColor1);
                    } else if (rawProgress >= 50 && rawProgress < 75) {
                        canvas.drawColor(mColor2);
                    } else {
                        canvas.drawColor(mColor3);
                    }
                }

                
                
                
                
                
                
                if ((rawProgress >= 0 && rawProgress <= 25)) {
                    float pct = (((rawProgress + 25) * 2) / 100f);
                    drawCircle(canvas, cx, cy, mColor1, pct);
                }
                if (rawProgress >= 0 && rawProgress <= 50) {
                    float pct = ((rawProgress * 2) / 100f);
                    drawCircle(canvas, cx, cy, mColor2, pct);
                }
                if (rawProgress >= 25 && rawProgress <= 75) {
                    float pct = (((rawProgress - 25) * 2) / 100f);
                    drawCircle(canvas, cx, cy, mColor3, pct);
                }
                if (rawProgress >= 50 && rawProgress <= 100) {
                    float pct = (((rawProgress - 50) * 2) / 100f);
                    drawCircle(canvas, cx, cy, mColor4, pct);
                }
                if ((rawProgress >= 75 && rawProgress <= 100)) {
                    float pct = (((rawProgress - 75) * 2) / 100f);
                    drawCircle(canvas, cx, cy, mColor1, pct);
                }
                if (mTriggerPercentage > 0 && drawTriggerWhileFinishing) {
                    
                    
                    
                    
                    canvas.restoreToCount(restoreCount);
                    restoreCount = canvas.save();
                    canvas.clipRect(mBounds);
                    drawTrigger(canvas, cx, cy);
                }
                
                ViewCompat.postInvalidateOnAnimation(mParent);
            } else {
                
                if (mTriggerPercentage > 0 && mTriggerPercentage <= 1.0) {
                    drawTrigger(canvas, cx, cy);
                }
            }
            canvas.restoreToCount(restoreCount);
        }

        private void drawTrigger(Canvas canvas, int cx, int cy) {
            mPaint.setColor(mColor1);
            
            
            canvas.drawRect(cx - cx * mTriggerPercentage, 0, cx + cx * mTriggerPercentage,
                mBounds.bottom, mPaint);
        }

        








        private void drawCircle(Canvas canvas, float cx, float cy, int color, float pct) {
            mPaint.setColor(color);
            canvas.save();
            canvas.translate(cx, cy);
            float radiusScale = INTERPOLATOR.getInterpolation(pct);
            canvas.scale(radiusScale, radiusScale);
            canvas.drawCircle(0, 0, cx, mPaint);
            canvas.restore();
        }

        


        void setBounds(int left, int top, int right, int bottom) {
            mBounds.left = left;
            mBounds.top = top;
            mBounds.right = right;
            mBounds.bottom = bottom;
        }
    }

    private static final class BakedBezierInterpolator implements Interpolator {
        private static final BakedBezierInterpolator INSTANCE = new BakedBezierInterpolator();

        public final static BakedBezierInterpolator getInstance() {
            return INSTANCE;
        }

        


        private BakedBezierInterpolator() {
            super();
        }

        









        private static final float[] VALUES = new float[] {
            0.0f, 0.0002f, 0.0009f, 0.0019f, 0.0036f, 0.0059f, 0.0086f, 0.0119f, 0.0157f, 0.0209f,
            0.0257f, 0.0321f, 0.0392f, 0.0469f, 0.0566f, 0.0656f, 0.0768f, 0.0887f, 0.1033f, 0.1186f,
            0.1349f, 0.1519f, 0.1696f, 0.1928f, 0.2121f, 0.237f, 0.2627f, 0.2892f, 0.3109f, 0.3386f,
            0.3667f, 0.3952f, 0.4241f, 0.4474f, 0.4766f, 0.5f, 0.5234f, 0.5468f, 0.5701f, 0.5933f,
            0.6134f, 0.6333f, 0.6531f, 0.6698f, 0.6891f, 0.7054f, 0.7214f, 0.7346f, 0.7502f, 0.763f,
            0.7756f, 0.7879f, 0.8f, 0.8107f, 0.8212f, 0.8326f, 0.8415f, 0.8503f, 0.8588f, 0.8672f,
            0.8754f, 0.8833f, 0.8911f, 0.8977f, 0.9041f, 0.9113f, 0.9165f, 0.9232f, 0.9281f, 0.9328f,
            0.9382f, 0.9434f, 0.9476f, 0.9518f, 0.9557f, 0.9596f, 0.9632f, 0.9662f, 0.9695f, 0.9722f,
            0.9753f, 0.9777f, 0.9805f, 0.9826f, 0.9847f, 0.9866f, 0.9884f, 0.9901f, 0.9917f, 0.9931f,
            0.9944f, 0.9955f, 0.9964f, 0.9973f, 0.9981f, 0.9986f, 0.9992f, 0.9995f, 0.9998f, 1.0f, 1.0f
        };

        private static final float STEP_SIZE = 1.0f / (VALUES.length - 1);

        @Override
        public float getInterpolation(float input) {
            if (input >= 1.0f) {
                return 1.0f;
            }

            if (input <= 0f) {
                return 0f;
            }

            int position = Math.min(
                    (int)(input * (VALUES.length - 1)),
                    VALUES.length - 2);

            float quantized = position * STEP_SIZE;
            float difference = input - quantized;
            float weight = difference / STEP_SIZE;

            return VALUES[position] + weight * (VALUES[position + 1] - VALUES[position]);
        }

    }
}

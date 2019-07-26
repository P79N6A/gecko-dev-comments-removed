




package org.mozilla.gecko.widget;

import org.mozilla.gecko.favicons.Favicons;
import org.mozilla.gecko.R;

import android.content.Context;
import android.graphics.Bitmap;
import android.graphics.Color;
import android.graphics.PorterDuff.Mode;
import android.graphics.drawable.Drawable;
import android.util.AttributeSet;
import android.widget.ImageView;





public class FaviconView extends ImageView {
    private Bitmap mIconBitmap;

    
    
    private Bitmap mUnscaledBitmap;

    
    
    
    private String mIconKey;

    private int mActualWidth;
    private int mActualHeight;

    
    private boolean mScalingExpected;

    public FaviconView(Context context, AttributeSet attrs) {
        super(context, attrs);
        setScaleType(ImageView.ScaleType.CENTER);
    }

    @Override
    protected void onSizeChanged(int xNew, int yNew, int xOld, int yOld){
        super.onSizeChanged(xNew, yNew, xOld, yOld);

        
        if (xNew == mActualHeight && yNew == mActualWidth) {
            return;
        }
        mActualWidth = xNew;
        mActualHeight = yNew;
        formatImage();
    }

    




    private void formatImage() {
        
        if (mIconBitmap == null || mActualWidth == 0 || mActualHeight == 0) {
            showNoImage();
            return;
        }

        if (mScalingExpected && mActualWidth != mIconBitmap.getWidth()) {
            scaleBitmap();
            
            mScalingExpected = false;
        }

        setImageBitmap(mIconBitmap);

        
        
        
        
        if (Math.abs(mIconBitmap.getWidth() - mActualWidth) > 3) {
            showBackground();
        } else {
            hideBackground();
        }
    }

    private void scaleBitmap() {
        
        
        int doubledSize = mIconBitmap.getWidth()*2;
        if (mActualWidth > doubledSize) {
            
            
            mIconBitmap = Bitmap.createScaledBitmap(mIconBitmap, doubledSize, doubledSize, true);
        } else {
            
            mIconBitmap = Bitmap.createScaledBitmap(mIconBitmap, mActualWidth, mActualWidth, true);
        }
    }

    



    private void showBackground() {
        int color = Favicons.getFaviconColor(mIconKey);
        if (color == -1) {
            hideBackground();
            return;
        }
        color = Color.argb(70, Color.red(color), Color.green(color), Color.blue(color));
        final Drawable drawable = getResources().getDrawable(R.drawable.favicon_bg);
        drawable.setColorFilter(color, Mode.SRC_ATOP);
        setBackgroundDrawable(drawable);
    }

    


    private void hideBackground() {
        setBackgroundResource(0);
    }

    












    private void updateImageInternal(Bitmap bitmap, String key, boolean allowScaling) {
        if (bitmap == null) {
            showDefaultFavicon();
            return;
        }

        
        if (mUnscaledBitmap == bitmap) {
            return;
        }
        mUnscaledBitmap = bitmap;
        mIconBitmap = bitmap;
        mIconKey = key;
        mScalingExpected = allowScaling;

        
        formatImage();
    }

    public void showDefaultFavicon() {
        setImageResource(R.drawable.favicon);
        hideBackground();
    }

    private void showNoImage() {
        setImageBitmap(null);
        hideBackground();
    }

    


    public void clearImage() {
        showNoImage();
        mUnscaledBitmap = null;
        mIconBitmap = null;
        mIconKey = null;
        mScalingExpected = false;
    }

    













    public void updateAndScaleImage(Bitmap bitmap, String key) {
        updateImageInternal(bitmap, key, true);
    }

    







    public void updateImage(Bitmap bitmap, String key) {
        updateImageInternal(bitmap, key, false);
    }

    public Bitmap getBitmap() {
        return mIconBitmap;
    }
}

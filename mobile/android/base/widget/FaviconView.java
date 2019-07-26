




package org.mozilla.gecko.widget;

import org.mozilla.gecko.Favicons;
import org.mozilla.gecko.R;

import android.content.Context;
import android.graphics.Bitmap;
import android.graphics.Color;
import android.graphics.PorterDuff.Mode;
import android.graphics.drawable.Drawable;
import android.os.AsyncTask;
import android.util.AttributeSet;
import android.widget.ImageView;





public class FaviconView extends ImageView {

    public FaviconView(Context context, AttributeSet attrs) {
        super(context, attrs);
        setScaleType(ImageView.ScaleType.CENTER);
    }

    



    public void updateImage(Bitmap bitmap, String key) {
        if (bitmap == null) {
            
            setImageDrawable(null);
            
            setBackgroundResource(R.drawable.favicon_bg);
        } else if (Favicons.getInstance().isLargeFavicon(bitmap)) {
            super.setImageBitmap(bitmap);
            
            setBackgroundResource(0);
        } else {
            super.setImageBitmap(bitmap);
            
            int color = Favicons.getInstance().getFaviconColor(bitmap, key);
            color = Color.argb(70, Color.red(color), Color.green(color), Color.blue(color));
            Drawable drawable = getResources().getDrawable(R.drawable.favicon_bg);
            drawable.setColorFilter(color, Mode.SRC_ATOP);
            setBackgroundDrawable(drawable);
        }
    }
}

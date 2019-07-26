




package org.mozilla.gecko.widget;

import org.mozilla.gecko.Favicons;
import org.mozilla.gecko.R;

import android.content.Context;
import android.graphics.Bitmap;
import android.util.AttributeSet;
import android.widget.ImageView;





public class FaviconView extends ImageView {

    public FaviconView(Context context, AttributeSet attrs) {
        super(context, attrs);
        setScaleType(ImageView.ScaleType.CENTER);
    }

    @Override
    public void setImageBitmap(Bitmap bitmap) {
        if (bitmap == null) {
            
            setImageDrawable(null);
            
            setBackgroundResource(R.drawable.favicon_bg);
        } else if (Favicons.getInstance().isLargeFavicon(bitmap)) {
            super.setImageBitmap(bitmap);
            
            setBackgroundResource(0);
        } else {
            super.setImageBitmap(bitmap);
            
            setBackgroundResource(R.drawable.favicon_bg);
        }
    }
}

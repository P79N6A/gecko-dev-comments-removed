



package org.mozilla.gecko.fxa.activities;

import android.content.res.Resources;
import android.graphics.Bitmap;
import android.graphics.drawable.BitmapDrawable;
import android.graphics.drawable.Drawable;
import android.preference.Preference;
import android.support.v4.graphics.drawable.RoundedBitmapDrawable;
import android.support.v4.graphics.drawable.RoundedBitmapDrawableFactory;
import com.squareup.picasso.Picasso;
import com.squareup.picasso.Target;
import org.mozilla.gecko.AppConstants;







public class PicassoPreferenceIconTarget implements Target {
    private final Preference preference;
    private final Resources resources;
    private final float cornerRadius;

    public PicassoPreferenceIconTarget(Resources resources, Preference preference) {
        this(resources, preference, 0);
    }

    public PicassoPreferenceIconTarget(Resources resources, Preference preference, float cornerRadius) {
        this.resources = resources;
        this.preference = preference;
        this.cornerRadius = cornerRadius;
    }

    @Override
    public void onBitmapLoaded(Bitmap bitmap, Picasso.LoadedFrom from) {
        
        if (!AppConstants.Versions.feature11Plus) {
            return;
        }

        final Drawable drawable;
        if (cornerRadius > 0) {
            final RoundedBitmapDrawable roundedBitmapDrawable;
            roundedBitmapDrawable = RoundedBitmapDrawableFactory.create(resources, bitmap);
            roundedBitmapDrawable.setCornerRadius(cornerRadius);
            roundedBitmapDrawable.setAntiAlias(true);
            drawable = roundedBitmapDrawable;
        } else {
            drawable = new BitmapDrawable(resources, bitmap);
        }
        preference.setIcon(drawable);
    }

    @Override
    public void onBitmapFailed(Drawable errorDrawable) {
        
        if (!AppConstants.Versions.feature11Plus) {
            return;
        }
        preference.setIcon(errorDrawable);
    }

    @Override
    public void onPrepareLoad(Drawable placeHolderDrawable) {
        
        if (!AppConstants.Versions.feature11Plus) {
            return;
        }
        preference.setIcon(placeHolderDrawable);
    }
}

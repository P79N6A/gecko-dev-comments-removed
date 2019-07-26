














package com.squareup.picasso;

import android.graphics.Bitmap;
import android.graphics.drawable.Drawable;

import static com.squareup.picasso.Picasso.LoadedFrom;










public interface Target {
  




  void onBitmapLoaded(Bitmap bitmap, LoadedFrom from);

  
  void onBitmapFailed(Drawable errorDrawable);

  
  void onPrepareLoad(Drawable placeHolderDrawable);
}

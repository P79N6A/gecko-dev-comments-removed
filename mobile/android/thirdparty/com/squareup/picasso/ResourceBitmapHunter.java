














package com.squareup.picasso;

import android.content.Context;
import android.content.res.Resources;
import android.graphics.Bitmap;
import android.graphics.BitmapFactory;
import java.io.IOException;

import static com.squareup.picasso.Picasso.LoadedFrom.DISK;

class ResourceBitmapHunter extends BitmapHunter {
  private final Context context;

  ResourceBitmapHunter(Context context, Picasso picasso, Dispatcher dispatcher, Cache cache,
      Stats stats, Action action) {
    super(picasso, dispatcher, cache, stats, action);
    this.context = context;
  }

  @Override Bitmap decode(Request data) throws IOException {
    Resources res = Utils.getResources(context, data);
    int id = Utils.getResourceId(res, data);
    return decodeResource(res, id, data);
  }

  @Override Picasso.LoadedFrom getLoadedFrom() {
    return DISK;
  }

  private Bitmap decodeResource(Resources resources, int id, Request data) {
    BitmapFactory.Options bitmapOptions = null;
    if (data.hasSize()) {
      bitmapOptions = new BitmapFactory.Options();
      bitmapOptions.inJustDecodeBounds = true;
      BitmapFactory.decodeResource(resources, id, bitmapOptions);
      calculateInSampleSize(data.targetWidth, data.targetHeight, bitmapOptions);
    }
    return BitmapFactory.decodeResource(resources, id, bitmapOptions);
  }
}

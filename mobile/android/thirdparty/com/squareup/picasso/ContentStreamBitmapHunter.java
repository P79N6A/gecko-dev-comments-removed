














package com.squareup.picasso;

import android.content.ContentResolver;
import android.content.Context;
import android.graphics.Bitmap;
import android.graphics.BitmapFactory;
import java.io.IOException;
import java.io.InputStream;

import static com.squareup.picasso.Picasso.LoadedFrom.DISK;

class ContentStreamBitmapHunter extends BitmapHunter {
  final Context context;

  ContentStreamBitmapHunter(Context context, Picasso picasso, Dispatcher dispatcher, Cache cache,
      Stats stats, Action action) {
    super(picasso, dispatcher, cache, stats, action);
    this.context = context;
  }

  @Override Bitmap decode(Request data)
      throws IOException {
    return decodeContentStream(data);
  }

  @Override Picasso.LoadedFrom getLoadedFrom() {
    return DISK;
  }

  protected Bitmap decodeContentStream(Request data) throws IOException {
    ContentResolver contentResolver = context.getContentResolver();
    BitmapFactory.Options options = null;
    if (data.hasSize()) {
      options = new BitmapFactory.Options();
      options.inJustDecodeBounds = true;
      InputStream is = null;
      try {
        is = contentResolver.openInputStream(data.uri);
        BitmapFactory.decodeStream(is, null, options);
      } finally {
        Utils.closeQuietly(is);
      }
      calculateInSampleSize(data.targetWidth, data.targetHeight, options);
    }
    InputStream is = contentResolver.openInputStream(data.uri);
    try {
      return BitmapFactory.decodeStream(is, null, options);
    } finally {
      Utils.closeQuietly(is);
    }
  }
}

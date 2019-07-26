














package com.squareup.picasso;

import android.content.Context;
import android.graphics.Bitmap;
import android.media.ExifInterface;
import android.net.Uri;
import java.io.IOException;

import static android.media.ExifInterface.ORIENTATION_NORMAL;
import static android.media.ExifInterface.ORIENTATION_ROTATE_180;
import static android.media.ExifInterface.ORIENTATION_ROTATE_270;
import static android.media.ExifInterface.ORIENTATION_ROTATE_90;
import static android.media.ExifInterface.TAG_ORIENTATION;

class FileBitmapHunter extends ContentStreamBitmapHunter {

  FileBitmapHunter(Context context, Picasso picasso, Dispatcher dispatcher, Cache cache,
      Stats stats, Action action) {
    super(context, picasso, dispatcher, cache, stats, action);
  }

  @Override Bitmap decode(Request data)
      throws IOException {
    setExifRotation(getFileExifRotation(data.uri));
    return super.decode(data);
  }

  static int getFileExifRotation(Uri uri) throws IOException {
    ExifInterface exifInterface = new ExifInterface(uri.getPath());
    int orientation = exifInterface.getAttributeInt(TAG_ORIENTATION, ORIENTATION_NORMAL);
    switch (orientation) {
      case ORIENTATION_ROTATE_90:
        return 90;
      case ORIENTATION_ROTATE_180:
        return 180;
      case ORIENTATION_ROTATE_270:
        return 270;
      default:
        return 0;
    }
  }
}

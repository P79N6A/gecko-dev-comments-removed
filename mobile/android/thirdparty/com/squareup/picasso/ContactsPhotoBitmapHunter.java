














package com.squareup.picasso;

import android.annotation.TargetApi;
import android.content.ContentResolver;
import android.content.Context;
import android.content.UriMatcher;
import android.graphics.Bitmap;
import android.graphics.BitmapFactory;
import android.net.Uri;
import android.provider.ContactsContract;

import java.io.IOException;
import java.io.InputStream;

import static android.os.Build.VERSION.SDK_INT;
import static android.os.Build.VERSION_CODES.ICE_CREAM_SANDWICH;
import static android.provider.ContactsContract.Contacts.openContactPhotoInputStream;
import static com.squareup.picasso.Picasso.LoadedFrom.DISK;

class ContactsPhotoBitmapHunter extends BitmapHunter {
  
  private static final int ID_LOOKUP = 1;
  
  private static final int ID_THUMBNAIL = 2;
  
  private static final int ID_CONTACT = 3;
  



  private static final int ID_DISPLAY_PHOTO = 4;

  private static final UriMatcher matcher;

  static {
    matcher = new UriMatcher(UriMatcher.NO_MATCH);
    matcher.addURI(ContactsContract.AUTHORITY, "contacts/lookup/*/#", ID_LOOKUP);
    matcher.addURI(ContactsContract.AUTHORITY, "contacts/lookup/*", ID_LOOKUP);
    matcher.addURI(ContactsContract.AUTHORITY, "contacts/#/photo", ID_THUMBNAIL);
    matcher.addURI(ContactsContract.AUTHORITY, "contacts/#", ID_CONTACT);
    matcher.addURI(ContactsContract.AUTHORITY, "display_photo/#", ID_DISPLAY_PHOTO);
  }

  final Context context;

  ContactsPhotoBitmapHunter(Context context, Picasso picasso, Dispatcher dispatcher, Cache cache,
      Stats stats, Action action) {
    super(picasso, dispatcher, cache, stats, action);
    this.context = context;
  }

  @Override Bitmap decode(Request data) throws IOException {
    InputStream is = null;
    try {
      is = getInputStream();
      return decodeStream(is, data);
    } finally {
      Utils.closeQuietly(is);
    }
  }

  @Override Picasso.LoadedFrom getLoadedFrom() {
    return DISK;
  }

  private InputStream getInputStream() throws IOException {
    ContentResolver contentResolver = context.getContentResolver();
    Uri uri = getData().uri;
    switch (matcher.match(uri)) {
      case ID_LOOKUP:
        uri = ContactsContract.Contacts.lookupContact(contentResolver, uri);
        if (uri == null) {
          return null;
        }
        
      case ID_CONTACT:
        if (SDK_INT < ICE_CREAM_SANDWICH) {
          return openContactPhotoInputStream(contentResolver, uri);
        } else {
          return ContactPhotoStreamIcs.get(contentResolver, uri);
        }
      case ID_THUMBNAIL:
      case ID_DISPLAY_PHOTO:
        return contentResolver.openInputStream(uri);
      default:
        throw new IllegalStateException("Invalid uri: " + uri);
    }
  }

  private Bitmap decodeStream(InputStream stream, Request data) throws IOException {
    if (stream == null) {
      return null;
    }
    BitmapFactory.Options options = null;
    if (data.hasSize()) {
      options = new BitmapFactory.Options();
      options.inJustDecodeBounds = true;
      InputStream is = getInputStream();
      try {
        BitmapFactory.decodeStream(is, null, options);
      } finally {
        Utils.closeQuietly(is);
      }
      calculateInSampleSize(data.targetWidth, data.targetHeight, options);
    }
    return BitmapFactory.decodeStream(stream, null, options);
  }

  @TargetApi(ICE_CREAM_SANDWICH)
  private static class ContactPhotoStreamIcs {
    static InputStream get(ContentResolver contentResolver, Uri uri) {
      return openContactPhotoInputStream(contentResolver, uri, true);
    }
  }
}

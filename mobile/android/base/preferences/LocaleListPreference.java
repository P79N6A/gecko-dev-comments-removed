



package org.mozilla.gecko.preferences;

import java.nio.ByteBuffer;
import java.text.Collator;
import java.util.Arrays;
import java.util.Collection;
import java.util.HashSet;
import java.util.Locale;
import java.util.Set;

import org.mozilla.gecko.AppConstants.Versions;
import org.mozilla.gecko.BrowserLocaleManager;
import org.mozilla.gecko.R;

import android.content.Context;
import android.graphics.Bitmap;
import android.graphics.Canvas;
import android.graphics.Paint;
import android.preference.ListPreference;
import android.text.TextUtils;
import android.util.AttributeSet;
import android.util.Log;

public class LocaleListPreference extends ListPreference {
    private static final String LOG_TAG = "GeckoLocaleList";

    













    private static class CharacterValidator {
        private static final int BITMAP_WIDTH = 32;
        private static final int BITMAP_HEIGHT = 48;

        private final Paint paint = new Paint();
        private final byte[] missingCharacter;

        public CharacterValidator(String missing) {
            this.missingCharacter = getPixels(drawBitmap(missing));
        }

        private Bitmap drawBitmap(String text){
            Bitmap b = Bitmap.createBitmap(BITMAP_WIDTH, BITMAP_HEIGHT, Bitmap.Config.ALPHA_8);
            Canvas c = new Canvas(b);
            c.drawText(text, 0, BITMAP_HEIGHT / 2, this.paint);
            return b;
        }

        private static byte[] getPixels(final Bitmap b) {
            final int byteCount;
            if (Versions.feature19Plus) {
                byteCount = b.getAllocationByteCount();
            } else {
                
                
                byteCount = b.getRowBytes() * b.getHeight();
            }

            final ByteBuffer buffer = ByteBuffer.allocate(byteCount);
            try {
                b.copyPixelsToBuffer(buffer);
            } catch (RuntimeException e) {
                
                
                
                
                if ("Buffer not large enough for pixels".equals(e.getMessage())) {
                    return buffer.array();
                }
                throw e;
            }

            return buffer.array();
        }

        public boolean characterIsMissingInFont(String ch) {
            byte[] rendered = getPixels(drawBitmap(ch));
            return Arrays.equals(rendered, missingCharacter);
        }
    }

    private volatile Locale entriesLocale;
    private final CharacterValidator characterValidator;

    public LocaleListPreference(Context context) {
        this(context, null);
    }

    public LocaleListPreference(Context context, AttributeSet attributes) {
        super(context, attributes);

        
        
        this.characterValidator = new CharacterValidator(" ");
        buildList();
    }

    private static final class LocaleDescriptor implements Comparable<LocaleDescriptor> {
        
        private static final Collator COLLATOR = Collator.getInstance(Locale.US);

        public final String tag;
        private final String nativeName;

        public LocaleDescriptor(String tag) {
            this(BrowserLocaleManager.parseLocaleCode(tag), tag);
        }

        public LocaleDescriptor(Locale locale, String tag) {
            this.tag = tag;

            final String displayName = locale.getDisplayName(locale);
            if (TextUtils.isEmpty(displayName)) {
                
                Log.w(LOG_TAG, "Display name is empty. Using " + locale.toString());
                this.nativeName = locale.toString();
                return;
            }

            
            
            
            final byte directionality = Character.getDirectionality(displayName.charAt(0));
            if (directionality == Character.DIRECTIONALITY_LEFT_TO_RIGHT) {
                this.nativeName = displayName.substring(0, 1).toUpperCase(locale) +
                                  displayName.substring(1);
                return;
            }

            this.nativeName = displayName;
        }

        public String getTag() {
            return this.tag;
        }

        public String getDisplayName() {
            return this.nativeName;
        }

        @Override
        public String toString() {
            return this.nativeName;
        }


        @Override
        public int compareTo(LocaleDescriptor another) {
            
            return COLLATOR.compare(this.nativeName, another.nativeName);
        }

        






        public boolean isUsable(CharacterValidator validator) {
            
            if (this.tag.equals("bn-IN")) {
                
                
                
                
                
                
                if (!this.nativeName.startsWith("বাংলা")) {
                    
                    
                    return false;
                }
            }

            
            
            
            
            if (this.tag.equals("or") ||
                this.tag.equals("pa-IN") ||
                this.tag.equals("gu-IN") ||
                this.tag.equals("bn-IN")) {
                if (validator.characterIsMissingInFont(this.nativeName.substring(0, 1))) {
                    return false;
                }
            }

            return true;
        }
    }

    





    private LocaleDescriptor[] getUsableLocales() {
        Collection<String> shippingLocales = BrowserLocaleManager.getPackagedLocaleTags(getContext());

        
        if (shippingLocales == null) {
            final String fallbackTag = BrowserLocaleManager.getFallbackLocaleTag();
            return new LocaleDescriptor[] { new LocaleDescriptor(fallbackTag) };
        }

        final int initialCount = shippingLocales.size();
        final Set<LocaleDescriptor> locales = new HashSet<LocaleDescriptor>(initialCount);
        for (String tag : shippingLocales) {
            final LocaleDescriptor descriptor = new LocaleDescriptor(tag);

            if (!descriptor.isUsable(this.characterValidator)) {
                Log.w(LOG_TAG, "Skipping locale " + tag + " on this device.");
                continue;
            }

            locales.add(descriptor);
        }

        final int usableCount = locales.size();
        final LocaleDescriptor[] descriptors = locales.toArray(new LocaleDescriptor[usableCount]);
        Arrays.sort(descriptors, 0, usableCount);
        return descriptors;
    }

    @Override
    protected void onDialogClosed(boolean positiveResult) {
        
        super.onDialogClosed(positiveResult);

        
        
        
        final Locale selectedLocale = getSelectedLocale();
        final Context context = getContext();
        BrowserLocaleManager.getInstance().updateConfiguration(context, selectedLocale);
    }

    private Locale getSelectedLocale() {
        final String tag = getValue();
        if (tag == null || tag.equals("")) {
            return Locale.getDefault();
        }
        return BrowserLocaleManager.parseLocaleCode(tag);
    }

    @Override
    public CharSequence getSummary() {
        final String value = getValue();

        if (TextUtils.isEmpty(value)) {
            return getContext().getString(R.string.locale_system_default);
        }

        
        
        return new LocaleDescriptor(value).getDisplayName();
    }

    private void buildList() {
        final Locale currentLocale = Locale.getDefault();
        Log.d(LOG_TAG, "Building locales list. Current locale: " + currentLocale);

        if (currentLocale.equals(this.entriesLocale) &&
            getEntries() != null) {
            Log.v(LOG_TAG, "No need to build list.");
            return;
        }

        final LocaleDescriptor[] descriptors = getUsableLocales();
        final int count = descriptors.length;

        this.entriesLocale = currentLocale;

        
        final String[] entries = new String[count + 1];
        final String[] values = new String[count + 1];

        entries[0] = getContext().getString(R.string.locale_system_default);
        values[0] = "";

        for (int i = 0; i < count; ++i) {
            entries[i + 1] = descriptors[i].getDisplayName();
            values[i + 1] = descriptors[i].getTag();
        }

        setEntries(entries);
        setEntryValues(values);
    }
}

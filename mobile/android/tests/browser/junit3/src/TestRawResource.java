


package org.mozilla.gecko;

import java.io.ByteArrayInputStream;
import java.io.IOException;
import java.io.InputStream;

import org.mozilla.gecko.util.RawResource;

import android.content.Context;
import android.content.res.Resources;
import android.test.mock.MockContext;
import android.test.mock.MockResources;





public class TestRawResource extends BrowserTestCase {
    private static final int RAW_RESOURCE_ID = 1;
    private static final String RAW_CONTENTS = "RAW";

    private static class TestContext extends MockContext {
        private final Resources resources;

        public TestContext() {
            resources = new TestResources();
        }

        @Override
        public Resources getResources() {
            return resources;
        }
    }

    




    private static class TestResources extends MockResources {
        @Override
        public InputStream openRawResource(int id) {
            if (id == RAW_RESOURCE_ID) {
                return new ByteArrayInputStream(RAW_CONTENTS.getBytes());
            }

            return null;
        }
    }

    public void testGet() {
        Context context = new TestContext();
        String result;

        try {
            result = RawResource.getAsString(context, RAW_RESOURCE_ID);
        } catch (IOException e) {
            result = null;
        }

        assertEquals(RAW_CONTENTS, result);
    }
}

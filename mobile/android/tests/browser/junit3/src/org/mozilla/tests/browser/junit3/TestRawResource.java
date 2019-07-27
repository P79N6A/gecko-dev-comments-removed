


package org.mozilla.tests.browser.junit3;

import android.content.Context;
import android.content.res.Resources;
import android.test.InstrumentationTestCase;
import android.test.mock.MockContext;
import android.test.mock.MockResources;
import org.mozilla.gecko.util.RawResource;

import java.io.ByteArrayInputStream;
import java.io.IOException;
import java.io.InputStream;





public class TestRawResource extends InstrumentationTestCase {
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

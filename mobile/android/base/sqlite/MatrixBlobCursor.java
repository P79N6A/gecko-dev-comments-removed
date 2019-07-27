
















package org.mozilla.gecko.sqlite;

import java.nio.ByteBuffer;
import java.util.ArrayList;

import org.mozilla.gecko.AppConstants;
import org.mozilla.gecko.mozglue.generatorannotations.WrapElementForJNI;

import android.database.AbstractCursor;
import android.database.CursorIndexOutOfBoundsException;
import android.util.Log;












public class MatrixBlobCursor extends AbstractCursor {
    private static final String LOGTAG = "GeckoMatrixCursor";

    private final String[] columnNames;
    private final int columnCount;

    private int rowCount;

    Object[] data;

    






    @WrapElementForJNI
    public MatrixBlobCursor(String[] columnNames, int initialCapacity) {
        this.columnNames = columnNames;
        this.columnCount = columnNames.length;

        if (initialCapacity < 1) {
            initialCapacity = 1;
        }

        this.data = new Object[columnCount * initialCapacity];
    }

    





    @WrapElementForJNI
    public MatrixBlobCursor(String[] columnNames) {
        this(columnNames, 16);
    }

    


    protected Object get(int column) {
        if (column < 0 || column >= columnCount) {
            throw new CursorIndexOutOfBoundsException("Requested column: "
                    + column + ", # of columns: " +  columnCount);
        }
        if (mPos < 0) {
            throw new CursorIndexOutOfBoundsException("Before first row.");
        }
        if (mPos >= rowCount) {
            throw new CursorIndexOutOfBoundsException("After last row.");
        }
        return data[mPos * columnCount + column];
    }

    






    public RowBuilder newRow() {
        rowCount++;
        int endIndex = rowCount * columnCount;
        ensureCapacity(endIndex);
        int start = endIndex - columnCount;
        return new RowBuilder(start, endIndex);
    }

    








    @WrapElementForJNI
    public void addRow(Object[] columnValues) {
        if (columnValues.length != columnCount) {
            throw new IllegalArgumentException("columnNames.length = "
                    + columnCount + ", columnValues.length = "
                    + columnValues.length);
        }

        int start = rowCount++ * columnCount;
        ensureCapacity(start + columnCount);
        System.arraycopy(columnValues, 0, data, start, columnCount);
    }

    








    @WrapElementForJNI
    public void addRow(Iterable<?> columnValues) {
        final int start = rowCount * columnCount;

        if (columnValues instanceof ArrayList<?>) {
            addRow((ArrayList<?>) columnValues, start);
            return;
        }

        final int end = start + columnCount;
        int current = start;

        ensureCapacity(end);
        final Object[] localData = data;
        for (Object columnValue : columnValues) {
            if (current == end) {
                
                throw new IllegalArgumentException(
                        "columnValues.size() > columnNames.length");
            }
            localData[current++] = columnValue;
        }

        if (current != end) {
            
            throw new IllegalArgumentException(
                    "columnValues.size() < columnNames.length");
        }

        
        rowCount++;
    }

    
    @WrapElementForJNI
    private void addRow(ArrayList<?> columnValues, int start) {
        final int size = columnValues.size();
        if (size != columnCount) {
            throw new IllegalArgumentException("columnNames.length = "
                    + columnCount + ", columnValues.size() = " + size);
        }

        final int end = start + columnCount;
        ensureCapacity(end);

        
        
        final Object[] localData = data;
        for (int i = 0; i < size; i++) {
            localData[start + i] = columnValues.get(i);
        }

        rowCount++;
    }

    



    private void ensureCapacity(final int size) {
        if (size <= data.length) {
            return;
        }

        final Object[] oldData = this.data;
        this.data = new Object[Math.max(size, data.length * 2)];
        System.arraycopy(oldData, 0, this.data, 0, oldData.length);
    }

    






    public class RowBuilder {
        private int index;
        private final int endIndex;

        RowBuilder(int index, int endIndex) {
            this.index = index;
            this.endIndex = endIndex;
        }

        






        public RowBuilder add(final Object columnValue) {
            if (index == endIndex) {
                throw new CursorIndexOutOfBoundsException("No more columns left.");
            }

            data[index++] = columnValue;
            return this;
        }
    }

    


    public void set(int column, Object value) {
        if (column < 0 || column >= columnCount) {
            throw new CursorIndexOutOfBoundsException("Requested column: "
                    + column + ", # of columns: " +  columnCount);
        }
        if (mPos < 0) {
            throw new CursorIndexOutOfBoundsException("Before first row.");
        }
        if (mPos >= rowCount) {
            throw new CursorIndexOutOfBoundsException("After last row.");
        }
        data[mPos * columnCount + column] = value;
    }

    
    @Override
    public int getCount() {
        return rowCount;
    }

    @Override
    public String[] getColumnNames() {
        return columnNames;
    }

    @Override
    public String getString(int column) {
        Object value = get(column);
        if (value == null) return null;
        return value.toString();
    }

    @Override
    public short getShort(int column) {
        final Object value = get(column);
        if (value == null) return 0;
        if (value instanceof Number) return ((Number) value).shortValue();
        return Short.parseShort(value.toString());
    }

    @Override
    public int getInt(int column) {
        Object value = get(column);
        if (value == null) return 0;
        if (value instanceof Number) return ((Number) value).intValue();
        return Integer.parseInt(value.toString());
    }

    @Override
    public long getLong(int column) {
        Object value = get(column);
        if (value == null) return 0;
        if (value instanceof Number) return ((Number) value).longValue();
        return Long.parseLong(value.toString());
    }

    @Override
    public float getFloat(int column) {
        Object value = get(column);
        if (value == null) return 0.0f;
        if (value instanceof Number) return ((Number) value).floatValue();
        return Float.parseFloat(value.toString());
    }

    @Override
    public double getDouble(int column) {
        Object value = get(column);
        if (value == null) return 0.0d;
        if (value instanceof Number) return ((Number) value).doubleValue();
        return Double.parseDouble(value.toString());
    }

    @Override
    public byte[] getBlob(int column) {
        Object value = get(column);
        if (value == null) return null;
        if (value instanceof byte[]) {
            return (byte[]) value;
        }

        if (value instanceof ByteBuffer) {
            final ByteBuffer bytes = (ByteBuffer) value;
            byte[] byteArray = new byte[bytes.remaining()];
            bytes.get(byteArray);
            return byteArray;
        }
        throw new UnsupportedOperationException("BLOB Object not of known type");
    }

    @Override
    public boolean isNull(int column) {
        return get(column) == null;
    }

    @Override
    protected void finalize() {
        if (AppConstants.DEBUG_BUILD) {
            if (!isClosed()) {
                Log.e(LOGTAG, "Cursor finalized without being closed", new RuntimeException("stack"));
            }
        }

        super.finalize();
    }
}






package org.mozilla.gecko;

public class PrivateTab extends Tab {
    public PrivateTab(int id, String url, boolean external, int parentId, String title) {
        super(id, url, external, parentId, title);
    }

    @Override
    protected void addHistory(final String uri) {}

    @Override
    protected void updateHistory(final String uri, final String title) {}

    @Override
    protected void saveThumbnailToDB() {}

    @Override
    public boolean isPrivate() {
        return true;
    }
}

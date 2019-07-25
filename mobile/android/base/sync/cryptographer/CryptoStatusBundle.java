




































package org.mozilla.gecko.sync.cryptographer;

public class CryptoStatusBundle {

    public enum CryptoStatus {
        OK,
        MISSING_KEYS,
        HMAC_VERIFY_FAIL,
        INVALID_JSON,
        INVALID_KEYS_BUNDLE,
        MISSING_SYNCKEY_OR_USER
    }

    private CryptoStatus status;
    private String json;

    public CryptoStatusBundle (CryptoStatus status, String json) {
       this.setStatus(status);
       this.setJson(json);
    }

    public CryptoStatus getStatus() {
        return status;
    }

    public void setStatus(CryptoStatus status) {
        this.status = status;
    }

    public String getJson() {
        return json;
    }

    public void setJson(String json) {
        this.json = json;
    }

}

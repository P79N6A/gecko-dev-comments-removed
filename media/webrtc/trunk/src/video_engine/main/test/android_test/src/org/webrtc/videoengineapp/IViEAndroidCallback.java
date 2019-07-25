









package org.webrtc.videoengineapp;

public interface IViEAndroidCallback {
    public int UpdateStats(int frameRateI, int bitRateI,
        int packetLoss, int frameRateO,
        int bitRateO);
}

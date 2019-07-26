









package org.webrtc.videoengineapp;

public interface IViEAndroidCallback {
    public int updateStats(int frameRateI, int bitRateI,
        int packetLoss, int frameRateO,
        int bitRateO);

    public int newIncomingResolution(int width, int height);
}

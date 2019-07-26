























#include "config.h"

#if ENABLE(WEB_AUDIO)

#include "core/platform/audio/HRTFPanner.h"

#include <algorithm>
#include "core/platform/audio/AudioBus.h"
#include "core/platform/audio/FFTConvolver.h"
#include "core/platform/audio/HRTFDatabase.h"
#include <wtf/MathExtras.h>
#include <wtf/RefPtr.h>

using namespace std;

namespace WebCore {



const double MaxDelayTimeSeconds = 0.002;

const int UninitializedAzimuth = -1;
const unsigned RenderingQuantum = 128;

HRTFPanner::HRTFPanner(float sampleRate, HRTFDatabaseLoader* databaseLoader)
    : Panner(PanningModelHRTF)
    , m_databaseLoader(databaseLoader)
    , m_sampleRate(sampleRate)
    , m_crossfadeSelection(CrossfadeSelection1)
    , m_azimuthIndex1(UninitializedAzimuth)
    , m_elevation1(0)
    , m_azimuthIndex2(UninitializedAzimuth)
    , m_elevation2(0)
    , m_crossfadeX(0)
    , m_crossfadeIncr(0)
    , m_convolverL1(fftSizeForSampleRate(sampleRate))
    , m_convolverR1(fftSizeForSampleRate(sampleRate))
    , m_convolverL2(fftSizeForSampleRate(sampleRate))
    , m_convolverR2(fftSizeForSampleRate(sampleRate))
    , m_delayLineL(MaxDelayTimeSeconds, sampleRate)
    , m_delayLineR(MaxDelayTimeSeconds, sampleRate)
    , m_tempL1(RenderingQuantum)
    , m_tempR1(RenderingQuantum)
    , m_tempL2(RenderingQuantum)
    , m_tempR2(RenderingQuantum)
{
    ASSERT(databaseLoader);
}

HRTFPanner::~HRTFPanner()
{
}

size_t HRTFPanner::fftSizeForSampleRate(float sampleRate)
{
    
    
    
    ASSERT(sampleRate >= 44100 && sampleRate <= 96000.0);
    return (sampleRate < 88200.0) ? 512 : 1024;
}

void HRTFPanner::reset()
{
    m_convolverL1.reset();
    m_convolverR1.reset();
    m_convolverL2.reset();
    m_convolverR2.reset();
    m_delayLineL.reset();
    m_delayLineR.reset();
}

int HRTFPanner::calculateDesiredAzimuthIndexAndBlend(double azimuth, double& azimuthBlend)
{
    
    
    if (azimuth < 0)
        azimuth += 360.0;

    HRTFDatabase* database = m_databaseLoader->database();
    ASSERT(database);

    int numberOfAzimuths = database->numberOfAzimuths();
    const double angleBetweenAzimuths = 360.0 / numberOfAzimuths;

    
    double desiredAzimuthIndexFloat = azimuth / angleBetweenAzimuths;
    int desiredAzimuthIndex = static_cast<int>(desiredAzimuthIndexFloat);
    azimuthBlend = desiredAzimuthIndexFloat - static_cast<double>(desiredAzimuthIndex);

    
    
    desiredAzimuthIndex = max(0, desiredAzimuthIndex);
    desiredAzimuthIndex = min(numberOfAzimuths - 1, desiredAzimuthIndex);
    return desiredAzimuthIndex;
}

void HRTFPanner::pan(double desiredAzimuth, double elevation, const AudioBus* inputBus, AudioBus* outputBus, size_t framesToProcess)
{
    unsigned numInputChannels = inputBus ? inputBus->numberOfChannels() : 0;

    bool isInputGood = inputBus &&  numInputChannels >= 1 && numInputChannels <= 2;
    ASSERT(isInputGood);

    bool isOutputGood = outputBus && outputBus->numberOfChannels() == 2 && framesToProcess <= outputBus->length();
    ASSERT(isOutputGood);

    if (!isInputGood || !isOutputGood) {
        if (outputBus)
            outputBus->zero();
        return;
    }

    HRTFDatabase* database = m_databaseLoader->database();
    ASSERT(database);
    if (!database) {
        outputBus->zero();
        return;
    }

    
    double azimuth = -desiredAzimuth;

    bool isAzimuthGood = azimuth >= -180.0 && azimuth <= 180.0;
    ASSERT(isAzimuthGood);
    if (!isAzimuthGood) {
        outputBus->zero();
        return;
    }

    
    
    const AudioChannel* inputChannelL = inputBus->channelByType(AudioBus::ChannelLeft);
    const AudioChannel* inputChannelR = numInputChannels > 1 ? inputBus->channelByType(AudioBus::ChannelRight) : 0;

    
    const float* sourceL = inputChannelL->data();
    const float* sourceR = numInputChannels > 1 ? inputChannelR->data() : sourceL;
    float* destinationL = outputBus->channelByType(AudioBus::ChannelLeft)->mutableData();
    float* destinationR = outputBus->channelByType(AudioBus::ChannelRight)->mutableData();

    double azimuthBlend;
    int desiredAzimuthIndex = calculateDesiredAzimuthIndexAndBlend(azimuth, azimuthBlend);

    
    if (m_azimuthIndex1 == UninitializedAzimuth) {
        m_azimuthIndex1 = desiredAzimuthIndex;
        m_elevation1 = elevation;
    }
    if (m_azimuthIndex2 == UninitializedAzimuth) {
        m_azimuthIndex2 = desiredAzimuthIndex;
        m_elevation2 = elevation;
    }

    
    
    
    const double fadeFrames = sampleRate() <= 48000 ? 2048 : 4096;

    
    if (!m_crossfadeX && m_crossfadeSelection == CrossfadeSelection1) {
        if (desiredAzimuthIndex != m_azimuthIndex1 || elevation != m_elevation1) {
            
            m_crossfadeIncr = 1 / fadeFrames;
            m_azimuthIndex2 = desiredAzimuthIndex;
            m_elevation2 = elevation;
        }
    }
    if (m_crossfadeX == 1 && m_crossfadeSelection == CrossfadeSelection2) {
        if (desiredAzimuthIndex != m_azimuthIndex2 || elevation != m_elevation2) {
            
            m_crossfadeIncr = -1 / fadeFrames;
            m_azimuthIndex1 = desiredAzimuthIndex;
            m_elevation1 = elevation;
        }
    }

    
    ASSERT(1UL << static_cast<int>(log2(framesToProcess)) == framesToProcess);
    ASSERT(framesToProcess >= RenderingQuantum);

    const unsigned framesPerSegment = RenderingQuantum;
    const unsigned numberOfSegments = framesToProcess / framesPerSegment;

    for (unsigned segment = 0; segment < numberOfSegments; ++segment) {
        
        HRTFKernel* kernelL1;
        HRTFKernel* kernelR1;
        HRTFKernel* kernelL2;
        HRTFKernel* kernelR2;
        double frameDelayL1;
        double frameDelayR1;
        double frameDelayL2;
        double frameDelayR2;
        database->getKernelsFromAzimuthElevation(azimuthBlend, m_azimuthIndex1, m_elevation1, kernelL1, kernelR1, frameDelayL1, frameDelayR1);
        database->getKernelsFromAzimuthElevation(azimuthBlend, m_azimuthIndex2, m_elevation2, kernelL2, kernelR2, frameDelayL2, frameDelayR2);

        bool areKernelsGood = kernelL1 && kernelR1 && kernelL2 && kernelR2;
        ASSERT(areKernelsGood);
        if (!areKernelsGood) {
            outputBus->zero();
            return;
        }

        ASSERT(frameDelayL1 / sampleRate() < MaxDelayTimeSeconds && frameDelayR1 / sampleRate() < MaxDelayTimeSeconds);
        ASSERT(frameDelayL2 / sampleRate() < MaxDelayTimeSeconds && frameDelayR2 / sampleRate() < MaxDelayTimeSeconds);

        
        double frameDelayL = (1 - m_crossfadeX) * frameDelayL1 + m_crossfadeX * frameDelayL2;
        double frameDelayR = (1 - m_crossfadeX) * frameDelayR1 + m_crossfadeX * frameDelayR2;

        
        unsigned offset = segment * framesPerSegment;
        const float* segmentSourceL = sourceL + offset;
        const float* segmentSourceR = sourceR + offset;
        float* segmentDestinationL = destinationL + offset;
        float* segmentDestinationR = destinationR + offset;

        
        m_delayLineL.setDelayFrames(frameDelayL);
        m_delayLineR.setDelayFrames(frameDelayR);
        m_delayLineL.process(segmentSourceL, segmentDestinationL, framesPerSegment);
        m_delayLineR.process(segmentSourceR, segmentDestinationR, framesPerSegment);

        bool needsCrossfading = m_crossfadeIncr;
        
        
        float* convolutionDestinationL1 = needsCrossfading ? m_tempL1.data() : segmentDestinationL;
        float* convolutionDestinationR1 = needsCrossfading ? m_tempR1.data() : segmentDestinationR;
        float* convolutionDestinationL2 = needsCrossfading ? m_tempL2.data() : segmentDestinationL;
        float* convolutionDestinationR2 = needsCrossfading ? m_tempR2.data() : segmentDestinationR;

        
        
        
        if (m_crossfadeSelection == CrossfadeSelection1 || needsCrossfading) {
            m_convolverL1.process(kernelL1->fftFrame(), segmentDestinationL, convolutionDestinationL1, framesPerSegment);
            m_convolverR1.process(kernelR1->fftFrame(), segmentDestinationR, convolutionDestinationR1, framesPerSegment);
        }

        if (m_crossfadeSelection == CrossfadeSelection2 || needsCrossfading) {
            m_convolverL2.process(kernelL2->fftFrame(), segmentDestinationL, convolutionDestinationL2, framesPerSegment);
            m_convolverR2.process(kernelR2->fftFrame(), segmentDestinationR, convolutionDestinationR2, framesPerSegment);
        }
        
        if (needsCrossfading) {
            
            float x = m_crossfadeX;
            float incr = m_crossfadeIncr;
            for (unsigned i = 0; i < framesPerSegment; ++i) {
                segmentDestinationL[i] = (1 - x) * convolutionDestinationL1[i] + x * convolutionDestinationL2[i];
                segmentDestinationR[i] = (1 - x) * convolutionDestinationR1[i] + x * convolutionDestinationR2[i];
                x += incr;
            }
            
            m_crossfadeX = x;

            if (m_crossfadeIncr > 0 && fabs(m_crossfadeX - 1) < m_crossfadeIncr) {
                
                m_crossfadeSelection = CrossfadeSelection2;
                m_crossfadeX = 1;
                m_crossfadeIncr = 0;
            } else if (m_crossfadeIncr < 0 && fabs(m_crossfadeX) < -m_crossfadeIncr) {
                
                m_crossfadeSelection = CrossfadeSelection1;
                m_crossfadeX = 0;
                m_crossfadeIncr = 0;
            }
        }
    }
}

double HRTFPanner::tailTime() const
{
    
    
    
    return MaxDelayTimeSeconds + (fftSize() / 2) / static_cast<double>(sampleRate());
}

double HRTFPanner::latencyTime() const
{
    
    
    return (fftSize() / 2) / static_cast<double>(sampleRate());
}

} 

#endif 

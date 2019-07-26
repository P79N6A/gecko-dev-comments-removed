























#include "HRTFPanner.h"
#include "HRTFDatabaseLoader.h"

#include "FFTConvolver.h"
#include "HRTFDatabase.h"
#include "WebAudioUtils.h"

using namespace std;
using namespace mozilla;
using mozilla::dom::WebAudioUtils;

namespace WebCore {



const double MaxDelayTimeSeconds = 0.002;

const int UninitializedAzimuth = -1;
const unsigned RenderingQuantum = 128;

HRTFPanner::HRTFPanner(float sampleRate, mozilla::TemporaryRef<HRTFDatabaseLoader> databaseLoader)
    : m_databaseLoader(databaseLoader)
    , m_sampleRate(sampleRate)
    , m_crossfadeSelection(CrossfadeSelection1)
    , m_azimuthIndex1(UninitializedAzimuth)
    , m_azimuthIndex2(UninitializedAzimuth)
    
    , m_crossfadeX(0)
    , m_crossfadeIncr(0)
    , m_convolverL1(HRTFElevation::fftSizeForSampleRate(sampleRate))
    , m_convolverR1(m_convolverL1.fftSize())
    , m_convolverL2(m_convolverL1.fftSize())
    , m_convolverR2(m_convolverL1.fftSize())
    , m_delayLineL(ceilf(MaxDelayTimeSeconds * sampleRate),
                   WebAudioUtils::ComputeSmoothingRate(0.02, sampleRate))
    , m_delayLineR(ceilf(MaxDelayTimeSeconds * sampleRate),
                   WebAudioUtils::ComputeSmoothingRate(0.02, sampleRate))
{
    MOZ_ASSERT(m_databaseLoader);
    MOZ_COUNT_CTOR(HRTFPanner);

    m_tempL1.SetLength(RenderingQuantum);
    m_tempR1.SetLength(RenderingQuantum);
    m_tempL2.SetLength(RenderingQuantum);
    m_tempR2.SetLength(RenderingQuantum);
}

HRTFPanner::~HRTFPanner()
{
    MOZ_COUNT_DTOR(HRTFPanner);
}

void HRTFPanner::reset()
{
    m_azimuthIndex1 = UninitializedAzimuth;
    m_azimuthIndex2 = UninitializedAzimuth;
    
    m_crossfadeSelection = CrossfadeSelection1;
    m_crossfadeX = 0.0f;
    m_crossfadeIncr = 0.0f;
    m_convolverL1.reset();
    m_convolverR1.reset();
    m_convolverL2.reset();
    m_convolverR2.reset();
    m_delayLineL.Reset();
    m_delayLineR.Reset();
}

int HRTFPanner::calculateDesiredAzimuthIndexAndBlend(double azimuth, double& azimuthBlend)
{
    
    
    if (azimuth < 0)
        azimuth += 360.0;

    HRTFDatabase* database = m_databaseLoader->database();
    MOZ_ASSERT(database);

    int numberOfAzimuths = database->numberOfAzimuths();
    const double angleBetweenAzimuths = 360.0 / numberOfAzimuths;

    
    double desiredAzimuthIndexFloat = azimuth / angleBetweenAzimuths;
    int desiredAzimuthIndex = static_cast<int>(desiredAzimuthIndexFloat);
    azimuthBlend = desiredAzimuthIndexFloat - static_cast<double>(desiredAzimuthIndex);

    
    
    desiredAzimuthIndex = max(0, desiredAzimuthIndex);
    desiredAzimuthIndex = min(numberOfAzimuths - 1, desiredAzimuthIndex);
    return desiredAzimuthIndex;
}

void HRTFPanner::pan(double desiredAzimuth, double elevation, const AudioChunk* inputBus, AudioChunk* outputBus, TrackTicks framesToProcess)
{
    unsigned numInputChannels =
        inputBus->IsNull() ? 0 : inputBus->mChannelData.Length();

    MOZ_ASSERT(numInputChannels <= 2);
    MOZ_ASSERT(framesToProcess <= inputBus->mDuration);

    bool isOutputGood = outputBus && outputBus->mChannelData.Length() == 2 && framesToProcess <= outputBus->mDuration;
    MOZ_ASSERT(isOutputGood);

    if (!isOutputGood) {
        if (outputBus)
            outputBus->SetNull(outputBus->mDuration);
        return;
    }

    HRTFDatabase* database = m_databaseLoader->database();
    if (!database) { 
        outputBus->SetNull(outputBus->mDuration);
        return;
    }

    
    double azimuth = -desiredAzimuth;

    bool isAzimuthGood = azimuth >= -180.0 && azimuth <= 180.0;
    MOZ_ASSERT(isAzimuthGood);
    if (!isAzimuthGood) {
        outputBus->SetNull(outputBus->mDuration);
        return;
    }

    
    

    
    const float* sourceL = numInputChannels > 0 ?
        static_cast<const float*>(inputBus->mChannelData[0]) : nullptr;
    const float* sourceR = numInputChannels > 1 ?
        static_cast<const float*>(inputBus->mChannelData[1]) : sourceL;
    float* destinationL =
        static_cast<float*>(const_cast<void*>(outputBus->mChannelData[0]));
    float* destinationR =
        static_cast<float*>(const_cast<void*>(outputBus->mChannelData[1]));

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

    
    MOZ_ASSERT(framesToProcess && 0 == (framesToProcess & (framesToProcess - 1)));
    MOZ_ASSERT(framesToProcess >= RenderingQuantum);

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
        MOZ_ASSERT(areKernelsGood);
        if (!areKernelsGood) {
            outputBus->SetNull(outputBus->mDuration);
            return;
        }

        MOZ_ASSERT(frameDelayL1 / sampleRate() < MaxDelayTimeSeconds && frameDelayR1 / sampleRate() < MaxDelayTimeSeconds);
        MOZ_ASSERT(frameDelayL2 / sampleRate() < MaxDelayTimeSeconds && frameDelayR2 / sampleRate() < MaxDelayTimeSeconds);

        
        double frameDelayL = (1 - m_crossfadeX) * frameDelayL1 + m_crossfadeX * frameDelayL2;
        double frameDelayR = (1 - m_crossfadeX) * frameDelayR1 + m_crossfadeX * frameDelayR2;

        
        unsigned offset = segment * framesPerSegment;
        const float* segmentSourceL = sourceL ? sourceL + offset : nullptr;
        const float* segmentSourceR = sourceR ? sourceR + offset : nullptr;
        float* segmentDestinationL = destinationL + offset;
        float* segmentDestinationR = destinationR + offset;

        
        m_delayLineL.Process(frameDelayL, &segmentSourceL, &segmentDestinationL, 1, framesPerSegment);
        m_delayLineR.Process(frameDelayR, &segmentSourceR, &segmentDestinationR, 1, framesPerSegment);

        bool needsCrossfading = m_crossfadeIncr;
        
        
        float* convolutionDestinationL1 = needsCrossfading ? m_tempL1.Elements() : segmentDestinationL;
        float* convolutionDestinationR1 = needsCrossfading ? m_tempR1.Elements() : segmentDestinationR;
        float* convolutionDestinationL2 = needsCrossfading ? m_tempL2.Elements() : segmentDestinationL;
        float* convolutionDestinationR2 = needsCrossfading ? m_tempR2.Elements() : segmentDestinationR;

        
        
        
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

int HRTFPanner::maxTailFrames() const
{
    
    
    
    
    
    
    
    return m_delayLineL.MaxDelayFrames() + fftSize();
}

} 

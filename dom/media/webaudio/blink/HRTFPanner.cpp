























#include "HRTFPanner.h"
#include "HRTFDatabaseLoader.h"

#include "FFTConvolver.h"
#include "HRTFDatabase.h"

using namespace std;
using namespace mozilla;
using dom::ChannelInterpretation;

namespace WebCore {



const double MaxDelayTimeSeconds = 0.002;

const int UninitializedAzimuth = -1;
const unsigned RenderingQuantum = WEBAUDIO_BLOCK_SIZE;

HRTFPanner::HRTFPanner(float sampleRate, already_AddRefed<HRTFDatabaseLoader> databaseLoader)
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
    , m_delayLine(MaxDelayTimeSeconds * sampleRate, 1.0)
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

size_t HRTFPanner::sizeOfIncludingThis(mozilla::MallocSizeOf aMallocSizeOf) const
{
    size_t amount = aMallocSizeOf(this);

    
    amount += m_convolverL1.sizeOfExcludingThis(aMallocSizeOf);
    amount += m_convolverR1.sizeOfExcludingThis(aMallocSizeOf);
    amount += m_convolverL2.sizeOfExcludingThis(aMallocSizeOf);
    amount += m_convolverR2.sizeOfExcludingThis(aMallocSizeOf);
    amount += m_delayLine.SizeOfExcludingThis(aMallocSizeOf);
    amount += m_tempL1.SizeOfExcludingThis(aMallocSizeOf);
    amount += m_tempL2.SizeOfExcludingThis(aMallocSizeOf);
    amount += m_tempR1.SizeOfExcludingThis(aMallocSizeOf);
    amount += m_tempR2.SizeOfExcludingThis(aMallocSizeOf);

    return amount;
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
    m_delayLine.Reset();
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

void HRTFPanner::pan(double desiredAzimuth, double elevation, const AudioChunk* inputBus, AudioChunk* outputBus)
{
#ifdef DEBUG
    unsigned numInputChannels =
        inputBus->IsNull() ? 0 : inputBus->mChannelData.Length();

    MOZ_ASSERT(numInputChannels <= 2);
    MOZ_ASSERT(inputBus->mDuration == WEBAUDIO_BLOCK_SIZE);
#endif

    bool isOutputGood = outputBus && outputBus->mChannelData.Length() == 2 && outputBus->mDuration == WEBAUDIO_BLOCK_SIZE;
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

    
    double frameDelaysL[WEBAUDIO_BLOCK_SIZE];
    double frameDelaysR[WEBAUDIO_BLOCK_SIZE];
    {
      float x = m_crossfadeX;
      float incr = m_crossfadeIncr;
      for (unsigned i = 0; i < WEBAUDIO_BLOCK_SIZE; ++i) {
        frameDelaysL[i] = (1 - x) * frameDelayL1 + x * frameDelayL2;
        frameDelaysR[i] = (1 - x) * frameDelayR1 + x * frameDelayR2;
        x += incr;
      }
    }

    
    m_delayLine.Write(*inputBus);
    
    
    m_delayLine.ReadChannel(frameDelaysL, outputBus, 0,
                            ChannelInterpretation::Speakers);
    m_delayLine.ReadChannel(frameDelaysR, outputBus, 1,
                            ChannelInterpretation::Speakers);
    m_delayLine.NextBlock();

    bool needsCrossfading = m_crossfadeIncr;

    
    float* convolutionDestinationL1 = needsCrossfading ? m_tempL1.Elements() : destinationL;
    float* convolutionDestinationR1 = needsCrossfading ? m_tempR1.Elements() : destinationR;
    float* convolutionDestinationL2 = needsCrossfading ? m_tempL2.Elements() : destinationL;
    float* convolutionDestinationR2 = needsCrossfading ? m_tempR2.Elements() : destinationR;

    
    

    if (m_crossfadeSelection == CrossfadeSelection1 || needsCrossfading) {
        m_convolverL1.process(kernelL1->fftFrame(), destinationL, convolutionDestinationL1, WEBAUDIO_BLOCK_SIZE);
        m_convolverR1.process(kernelR1->fftFrame(), destinationR, convolutionDestinationR1, WEBAUDIO_BLOCK_SIZE);
    }

    if (m_crossfadeSelection == CrossfadeSelection2 || needsCrossfading) {
        m_convolverL2.process(kernelL2->fftFrame(), destinationL, convolutionDestinationL2, WEBAUDIO_BLOCK_SIZE);
        m_convolverR2.process(kernelR2->fftFrame(), destinationR, convolutionDestinationR2, WEBAUDIO_BLOCK_SIZE);
    }

    if (needsCrossfading) {
        
        float x = m_crossfadeX;
        float incr = m_crossfadeIncr;
        for (unsigned i = 0; i < WEBAUDIO_BLOCK_SIZE; ++i) {
            destinationL[i] = (1 - x) * convolutionDestinationL1[i] + x * convolutionDestinationL2[i];
            destinationR[i] = (1 - x) * convolutionDestinationR1[i] + x * convolutionDestinationR2[i];
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

int HRTFPanner::maxTailFrames() const
{
    
    
    
    
    
    
    
    return m_delayLine.MaxDelayTicks() + fftSize();
}

} 

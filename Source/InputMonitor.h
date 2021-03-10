/*
  ==============================================================================

    InputMonitor.h

    //DN:  A simple AudioSource class where we can send the audio input in buffer 
    form, to be read from by our main mixer
  ==============================================================================
*/

#pragma once


#include <JuceHeader.h>


class InputMonitor : public juce::AudioSource
{
public:
    InputMonitor()
    {
        inputBuffer.reset(new juce::AudioBuffer<float>(2, 0));
    }
    ~InputMonitor(){}

    void prepareToPlay(int samplesPerBlockExpected, double sampleRate){}

    void releaseResources(){}

    void getNextAudioBlock(const juce::AudioSourceChannelInfo& bufferToFill)
    {

        int loopBufferSize = inputBuffer->getNumSamples();
        int maxInChannels = inputBuffer->getNumChannels();
        int maxOutChannels = bufferToFill.buffer->getNumChannels();

        if (loopBufferSize > 0 && maxInChannels > 0)
        {
            for (int i = 0; i < maxOutChannels; ++i)
            {
                auto writer = bufferToFill.buffer->getWritePointer(i, bufferToFill.startSample);

                for (auto sample = 0; sample < bufferToFill.numSamples; ++sample)
                {
                    writer[sample] = inputBuffer->getSample(i % maxInChannels, juce::jmin(sample, loopBufferSize)) * gain;
                }
            }
        }
    }

    void setBuffer(juce::AudioSampleBuffer* newBuffer)
    {
        inputBuffer.reset(newBuffer);
    }

    void setGain(double newGain)
    {
        gain = newGain;
    }

private:
    std::unique_ptr<juce::AudioBuffer<float>> inputBuffer;
    double gain = 1.0;
};
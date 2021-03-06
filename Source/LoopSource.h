/*
  ==============================================================================

    LoopAudioTransportSource.h

    DN:  This is a class based loosely on JUCE's AudioTransportSource, to achieve 
    similar function, but allow sample-level control of the playback using
    an AudioBuffer. This allows us to offset the audio start from the start of the 
    master Loop, and play silence after audio ends until the master Loop is over.
    Playback will loop until stopped, the length/sync will be consistent between tracks 
    (masterLoopLength, calculated via setting tempo, time signature, and # measures).
    The saved audio buffer in memory will playback only
    during the appropriate section of the master Loop by respecting the 
    fileStartOffset and the length of what's in the loopBuffer.

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>

class LoopSource: public juce::PositionableAudioSource, public juce::ChangeBroadcaster
{
public:
    LoopSource()
    {
        masterLoopTempo = 120; 
        masterLoopBeatsPerLoop = 16;
        calcMasterLoopLength();
        loopBuffer.reset(new juce::AudioBuffer<float>(2, 0));  //DN: just set up a length 0 buffer so silent playback can happen 
    }

    ~LoopSource()
    {
    }

    //==============================================================================
    void setNextReadPosition(juce::int64 newPosition) override
    {
        jassert(newPosition >= 0);

        position = newPosition;
    }

    juce::int64 getNextReadPosition() const override { return static_cast<juce::int64> (position); }

    //required by the base class
    juce::int64 getTotalLength() const override 
    { 
        return masterLoopLength;
    }

    juce::int64 getMasterLoopLength()
    {
        return masterLoopLength;
    }

    bool isLooping() const override { return true; };


    bool isPlaying()
    {
        return playing;
    }

    int getPosition()
    {
        return position;
    }

    //==============================================================================
    void prepareToPlay(int samplesPerBlockExpected, double newSampleRate)
    {
        const juce::ScopedLock sl(callbackLock);

        sampleRate = newSampleRate;
        calcMasterLoopLength();  //DN:  if sample rate changes, need to recalc masterLoopLength
    }


    //Call this for all tracks to keep them in sync
    void setMasterLoop(int tempo, int beatsPerLoop)
    {
        masterLoopTempo = tempo;
        masterLoopBeatsPerLoop = beatsPerLoop;
        calcMasterLoopLength();
    }

    //DN: calculates the length in samples of the master loop (important - masterLoopLength is used in the audio processing block)
    void calcMasterLoopLength()
    {
        double lengthInSeconds = (double)(60.0f / masterLoopTempo) * masterLoopBeatsPerLoop;
        masterLoopLength = int((lengthInSeconds * sampleRate) + 0.5f); //the 0.5 is to account for the integer cast, allows for correct rounding
    }
    
    
    void releaseResources() override {}

    void setBuffer(juce::AudioSampleBuffer* newBuffer)
    {
        loopBuffer.reset(newBuffer);
    }

    void start(int position)
    {
        if (!playing && position < masterLoopLength)
        {
            {
                const juce::ScopedLock sl(callbackLock);
                playing = true;
                stopped = false;
            }
            sendChangeMessage();
        }
    }

    void stop()
    {
        beginningOfFile = false;
        if (playing)
        {
            playing = false;
            sendChangeMessage();
        }
    }

    void startRecording()
    {
        recording = true;
    }

    void stopRecording()
    {
        recording = false;
    }


    void getNextAudioBlock(const juce::AudioSourceChannelInfo& bufferToFill) override
    {
        const juce::ScopedLock sl(callbackLock);
        
        auto hitLoopEnd = false;

        bufferToFill.clearActiveBufferRegion();  //DN: start with silence, so if we need it it's already there

        if (!stopped)
        {
            int loopBufferSize = loopBuffer->getNumSamples();
            int maxInChannels = loopBuffer->getNumChannels();

            int maxOutChannels = bufferToFill.buffer->getNumChannels();

            int pos = position;
            for (int i = 0; i < maxOutChannels; ++i)
            {
                auto writer = bufferToFill.buffer->getWritePointer(i, bufferToFill.startSample);

                pos = position; //need to reset/use a separate counter for each channel
                for (auto sample = 0; sample < bufferToFill.numSamples; ++sample)
                {
                    if (pos == masterLoopLength)
                    {
                        pos = 0;

                        // AF: Update position only at last iteration of the outer loop
                        // to prevent channels for being out of sync
                        if (i == maxOutChannels - 1)
                        {
                            hitLoopEnd = true;
                        }
                    }

                    //DN:  we only want to read the fileBuffer to output if it's not currently being recorded over,
                    // and the position is within the range it should be played back
                    //ignore null warning - should never be null since we allocate a dummy buffer in the constructor
                    if (!recording && (pos > fileStartOffset) && (pos < (loopBufferSize + fileStartOffset)))
                        writer[sample] = loopBuffer->getSample(i%maxInChannels,pos-fileStartOffset);

                    pos++;
                }
            }

            // Check for beginning of file
            if (hitLoopEnd)
                beginningOfFile = true;
            else if (position > (sampleRate/3))  //DN: delay where flag will stay true
                beginningOfFile = false;

            position = pos;

            if (!playing)
            {
                beginningOfFile = false;
                // DN: someone hit "stop", so fade out the last block we just filled
                for (int i = bufferToFill.buffer->getNumChannels(); --i >= 0;)
                    bufferToFill.buffer->applyGainRamp(i, bufferToFill.startSample, juce::jmin(256, bufferToFill.numSamples), 1.0f, 0.0f);

                if (bufferToFill.numSamples > 256)
                    bufferToFill.buffer->clear(bufferToFill.startSample + 256, bufferToFill.numSamples - 256);
            }

            stopped = !playing;
        }
    }

    bool readyToRecord()
    {
        if (beginningOfFile)
            return true;

        return false;
    }

    void setBeginningOfFile(bool boolean)
    {
        beginningOfFile = boolean;
    }

    void setFileStartOffset(int newStartOffset)
    {
        fileStartOffset = newStartOffset;
    }

    void reverseAudio()
    {
        loopBuffer->reverse(0, loopBuffer->getNumSamples());
    }

    juce::AudioBuffer<float>* getLoopBuffer()
    {
        return loopBuffer.get();
    }

    int getBpm()
    {
        return masterLoopTempo;
    }

private:
    //==============================================================================
    std::unique_ptr<juce::AudioBuffer<float>> loopBuffer;  //DN: array containing the audio we've read into memory in AudioTrack.h stopRecording()
    int position = 0; //DN:  important, this tracks our position as we iterate over the masterLoopLength, which can be longer and start before the audio file
    int fileStartOffset = 0;  //DN:  set this to delay when the contents of the loopBuffer play back, relative to position 0
    
    bool stopped = true, playing = false, recording = false, playAcrossAllChannels = true;
    double sampleRate = 44100.0;

    // AF: Flag to set when it's ready to record
    bool beginningOfFile = false;

    juce::CriticalSection callbackLock;

    int masterLoopTempo;
    int masterLoopBeatsPerLoop;
    int masterLoopLength; //DN: length in SAMPLES of the loop, so this depends on tempo, measures ,timesig, and sample Rate


};
/*
  ==============================================================================

    LoopAudioTransportSource.h
    Created: 3 Feb 2021 10:37:23am
    Author:  dnego


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
        masterLoopMeasures = 2;
        masterLoopBeatsPerMeasure = 4;
        calcMasterLoopLength();
    }

    ~LoopSource()
    {
        delete loopBuffer;
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

    //==============================================================================
    void prepareToPlay(int samplesPerBlockExpected, double newSampleRate)
    {
        const juce::ScopedLock sl(callbackLock);

        sampleRate = newSampleRate;
        calcMasterLoopLength();  //DN:  if sample rate changes, need to recalc masterLoopLength
    }


    //Call this for all tracks to keep them in sync
    void setMasterLoop(int tempo, int measures, int beatsPerMeasure)
    {
        int masterLoopTempo = tempo;
        int masterLoopMeasures = measures;
        int masterLoopBeatsPerMeasure = beatsPerMeasure;
    }

    //DN: calculates the length in samples of the master loop (important - masterLoopLength is used in the audio processing block)
    void calcMasterLoopLength()
    {
        int totalBeats = masterLoopBeatsPerMeasure * masterLoopMeasures;
        double lengthInSeconds = (double)(60.0f / masterLoopTempo) * totalBeats;
        masterLoopLength = int((lengthInSeconds * sampleRate) + 0.5f); //the 0.5 is to account for the integer cast, allows for correct rounding
    }
    
    
    void releaseResources() override {}

    void setBuffer(juce::AudioSampleBuffer* newBuffer)
    {
        if (loopBuffer != nullptr)
            delete loopBuffer;
        loopBuffer = newBuffer;
    }

    void start()
    {
        if ((!playing) && loopBuffer != nullptr)
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
        if (playing)
        {
            playing = false;

            int n = 500;
            //while (--n >= 0 && !stopped)
            //    juce::Thread::sleep(2);

            sendChangeMessage();
        }
    }


    void getNextAudioBlock(const juce::AudioSourceChannelInfo& bufferToFill) override
    {
        const juce::ScopedLock sl(callbackLock);

        bufferToFill.clearActiveBufferRegion();  //DN: start with silence, so if we need it it's already there

        if (loopBuffer != nullptr && !stopped)
        {
            const int loopBufferSize = loopBuffer->getNumSamples();

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
                        pos = 0;

                    if ((pos > fileStartOffset) && (pos < (loopBufferSize + fileStartOffset)))
                        writer[sample] = loopBuffer->getSample(i%maxInChannels,pos-fileStartOffset);

                    pos++;
                }
            }

            position = pos;


            if (!playing)
            {
                // DN: someone hit "stop", so fade out the last block we just filled
                for (int i = bufferToFill.buffer->getNumChannels(); --i >= 0;)
                    bufferToFill.buffer->applyGainRamp(i, bufferToFill.startSample, juce::jmin(256, bufferToFill.numSamples), 1.0f, 0.0f);

                if (bufferToFill.numSamples > 256)
                    bufferToFill.buffer->clear(bufferToFill.startSample + 256, bufferToFill.numSamples - 256);
            }

            stopped = !playing;


            //DN: this was for gain control changes, to avoid pops and clicks I think - might need later if we add a gain slider
           // for (int i = bufferToFill.buffer->getNumChannels(); --i >= 0;)
           //     info.buffer->applyGainRamp(i, info.startSample, info.numSamples, lastGain, gain);
        }
    }

    void setFileStartOffset(int newStartOffset)
    {
        fileStartOffset = newStartOffset;
    }

private:
    //==============================================================================
    juce::AudioBuffer<float>* loopBuffer = nullptr;  //DN: array containing the audio we've read into memory in AudioTrack.h stopRecording()
    int position = 0; //DN:  important, this tracks our position as we iterate over the masterLoopLength, which can be longer and start before the audio file
    int fileStartOffset = 0;  //DN:  set this when we want a file to always playback from the position it was recorded in masterLoop, rather than pos 0
    
    bool stopped = true, playing = false, playAcrossAllChannels = true;
    double sampleRate = 44100.0;
    

    juce::CriticalSection callbackLock;

    int masterLoopTempo;
    int masterLoopMeasures;
    int masterLoopBeatsPerMeasure;

    int masterLoopLength; //DN: length in SAMPLES of the loop, so this depends on tempo, measures ,timesig, and sample Rate


};
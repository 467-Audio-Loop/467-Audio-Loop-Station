/*
  ==============================================================================

    Metronome.h
    Created: 1 Mar 2021 11:38:01am
    Author:  Antonio Florencio

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
#include "../JuceLibraryCode/JuceHeader.h"

class Metronome : public juce::AudioSource
{
public:
    // AF: Constructor
    Metronome()
    {
        mFormatManager.registerBasicFormats();

        juce::File myFile{ juce::File::getSpecialLocation(juce::File::SpecialLocationType::userDesktopDirectory) }; // Have to change this to the actual path
        auto mySamples = myFile.findChildFiles(juce::File::TypesOfFileToFind::findFiles, true, "metronome_click.wav");

        jassert(mySamples[0].exists());

        auto formatReader = mFormatManager.createReaderFor(mySamples[0]);

        pMetronomeSample.reset(new juce::AudioFormatReaderSource(formatReader, true));

        mUpdateInterval = 60.0 / mBpm * mSampleRate;
    }

    void prepareToPlay(int samplesPerBlock, double sampleRate)
    {
        mSampleRate = sampleRate;
        mUpdateInterval = 60.0 / mBpm * mSampleRate;

        if (pMetronomeSample != nullptr)
        {
            pMetronomeSample->prepareToPlay(samplesPerBlock, sampleRate);
            DBG("file loaded");
        }
    }

    void getNextAudioBlock(const juce::AudioSourceChannelInfo& bufferToFill)
    {
        mUpdateInterval = 60.0 / mBpm * mSampleRate;
        const auto bufferSize = bufferToFill.numSamples;

        mTotalSamples += bufferSize;

        mSamplesRemaining = mTotalSamples % mUpdateInterval;

        if ((mSamplesRemaining + bufferSize) >= mUpdateInterval)
        {
            const auto timeToStartPlaying = mUpdateInterval - mSamplesRemaining;
            pMetronomeSample->setNextReadPosition(0);

            bool played = false;

            for (auto sample = 0; sample < bufferSize; sample++)
            {
                if (sample == timeToStartPlaying && state == Playing)
                {
                    played = true;
                    pMetronomeSample->getNextAudioBlock(bufferToFill);
                }
            }

            // AF: Need this in case timeToStartPlaying ends up being exactly == bufferSize
            if (played == false && state == Playing)
            {
                played = true;
                pMetronomeSample->getNextAudioBlock(bufferToFill);
            }
        }

      /* if (pMetronomeSample->getNextReadPosition() != 0 && state == Playing)
        {
           pMetronomeSample->getNextAudioBlock(bufferToFill);
        }    */  
    }

    // AF: Getter
    int getBpm() {
        return mBpm;
    }

    // AF: Setter
    void setBpm(int newBpm) {
        mBpm = newBpm;
        reset();
    }

    void reset()
    {
        mTotalSamples = 0;
    }

    enum mPlayState
    {
        Playing,
        Stopped
    };

    mPlayState getState()
    {
        return state;
    }

    void setState(mPlayState newState)
    {
        state = newState;
    }

    void start()
    {
        gain = 1.0;
        state = Playing;
    }

    void stop()
    {
        state = Stopped;
        reset();
    }

    void releaseResources()
    {
        // AF: Required override by parent class AudioSource
    }

    void setGain(double newGain)
    {
        gain = newGain;
    }

private:
    int mTotalSamples{ 0 };
    double mSampleRate{ 0 };
    int mBpm{ 120 };
    int mUpdateInterval{ 0 };
    int mSamplesRemaining{ 0 };
    double gain{ 1.0 };

    mPlayState state{ Stopped };

    juce::AudioFormatManager mFormatManager;
    std::unique_ptr<juce::AudioFormatReaderSource> pMetronomeSample{ nullptr };
};

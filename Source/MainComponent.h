#pragma once

#include "AudioTrack.h"

//==============================================================================
/*
    This component lives inside our window, and this is where you should put all
    your controls and content.
*/
class MainComponent  : public juce::AudioAppComponent,
                       public juce::ChangeListener
{
public:
    //==============================================================================
    MainComponent();
    ~MainComponent() override;

    //==============================================================================
    void prepareToPlay (int samplesPerBlockExpected, double sampleRate) override;
    void getNextAudioBlock (const juce::AudioSourceChannelInfo& bufferToFill) override;
    void releaseResources() override;

    //==============================================================================
    void paint (juce::Graphics& g) override;
    void resized() override;
    void changeListenerCallback(juce::ChangeBroadcaster* source) override;


private:

    // AF: enum responsible to change the states of play and stop buttons
    enum TransportState
    {
        Stopped,
        Starting,
        Playing,
        Stopping
    };

    // AF: Function that changes the state of the buttons
    void changeState(TransportState newState);

    // AF: Functions that deal with the buttons being clicked
    void playButtonClicked();
    void stopButtonClicked();

    //==============================================================================
    // AF: Added method that returns true if any tracks are currently playing
    bool trackCurrentlyPlaying();

    //==============================================================================

    juce::Random random;
    juce::AudioDeviceSelectorComponent audioSetupComp;

    juce::TextButton playButton{ "Play" };
    juce::TextButton stopButton{ "Stop" };


    AudioTrack track1{ "Loopstation Track1.wav" };
    AudioTrack track2{ "Loopstation Track2.wav" };
    AudioTrack track3{ "Loopstation Track3.wav" };
    AudioTrack track4{ "Loopstation Track4.wav" };

    juce::MixerAudioSource mixer;

    juce::TextButton track1RecordButton{ "Record" };
    juce::TextButton track2RecordButton{ "Record" };
    juce::TextButton track3RecordButton{ "Record" };
    juce::TextButton track4RecordButton{ "Record" };


    TransportState state;


    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MainComponent)
};

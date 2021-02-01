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
    void startRecording();
    void stopRecording();
    void playButtonClicked();
    void stopButtonClicked();
    void recordingSaved();

    //==============================================================================

    juce::Random random;
    juce::AudioDeviceSelectorComponent audioSetupComp;

    juce::TextButton playButton{ "Play" };
    juce::TextButton stopButton{ "Stop" };

    RecordingThumbnail track1RecordingThumbnail;
    AudioRecorder track1Recorder{ track1RecordingThumbnail.getAudioThumbnail() };
    juce::TextButton track1RecordButton{ "Record" };
    juce::TextButton track2RecordButton{ "Record" };


    juce::AudioFormatManager formatManager;
    std::unique_ptr<juce::AudioFormatReaderSource> track1ReaderSource;
    juce::AudioTransportSource track1TransportSource;
    std::unique_ptr<juce::AudioFormatReaderSource> track2ReaderSource;
    juce::AudioTransportSource track2TransportSource;
    TransportState state;

    juce::File track1LastRecording;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MainComponent)
};

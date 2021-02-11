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
    void saveButtonClicked();
    void initializeButtonClicked();
    void savedLoopSelected();

    //==============================================================================
    // AF: Method that returns true if any tracks are currently playing
    bool trackCurrentlyPlaying();
    // AF: Returns true if any tracks are currently recording
    bool trackCurrentlyRecording();

    //==============================================================================

    juce::AudioDeviceSelectorComponent audioSetupComp;

    // Global controls
    juce::TextButton playButton{ "Play" };
    juce::TextButton stopButton{ "Stop" };
    juce::Label savedLoopsLabel{ "savedLoopLabel","Project Currently Loaded" };
    juce::ComboBox savedLoopsDropdown{ "savedLoopsDropdown" };
    DirectoryTree savedLoopDirTree;
    juce::StringArray savedLoopFolderNames;
    juce::TextButton saveButton{ "Save Project" };
    juce::AlertWindow saveProjectWindow{"Save Project","Enter the name of your Loop Project:",juce::AlertWindow::AlertIconType::NoIcon};
    juce::TextButton initializeButton{ "New Project" };
    juce::AlertWindow initializeProjectWarning{ "Start Over","If you have not given this project a name, it will be lost.  Continue?",juce::AlertWindow::AlertIconType::WarningIcon };


    AudioTrack track1;
    AudioTrack track2;
    AudioTrack track3;
    AudioTrack track4;

    juce::MixerAudioSource mixer;

    juce::TextButton track1RecordButton{ "Record" };
    juce::TextButton track2RecordButton{ "Record" };
    juce::TextButton track3RecordButton{ "Record" };
    juce::TextButton track4RecordButton{ "Record" };


    TransportState state;

    InputSource inputAudio;


    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MainComponent)
};

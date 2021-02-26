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
    void resizeTrack(juce::Rectangle<int> trackArea); //DN:  added to DRY up resize function
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

    // Loading/Saving Audio to from tracks
    void initializeTempWAVs();
    void refreshAudioReferences();
    void redrawAndBufferAudio();

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
    juce::Label savedLoopsLabel{ "savedLoopLabel","Current Project File" };
    juce::ComboBox savedLoopsDropdown{ "savedLoopsDropdown" };
    DirectoryTree savedLoopDirTree;
    juce::TextButton saveButton{ "Save Project" };
    juce::TextButton initializeButton{ "New Project" };
    juce::AlertWindow unsavedProgressWarning{ "Unsaved Progress Warning","You will lose any unsaved progress.  Continue?",juce::AlertWindow::AlertIconType::WarningIcon };
    bool unsavedChanges = false; //DN: determines whether to warn about unsaved progress when switching projects
    int currentProjectListID = 0; //DN: keep track of where we are in the project list.  Update this when changing the dropdown

    // AF: Channels
    int maxOutputChannels = 0;
    int maxInputChannels = 0;

    juce::OwnedArray<AudioTrack> tracksArray;

    juce::MixerAudioSource mixer;

    TransportState state;

    InputSource inputAudio;


    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MainComponent)
};

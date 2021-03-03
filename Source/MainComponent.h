#pragma once

#include "AudioTrack.h"
#include "Metronome.h"



//==============================================================================
/*
    This component lives inside our window, and this is where you should put all
    your controls and content.
*/
class MainComponent  : public juce::AudioAppComponent,
                       public juce::ChangeListener,
                       public juce::KeyListener,
    public juce::TextEditor::Listener
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


    //==============================================================================
    // AF: Metronome
    void metronomeButtonClicked();

    //==============================================================================
    // AF: Text Box Listeners
    void textEditorReturnKeyPressed(juce::TextEditor &textEditor) override;
    void textEditorFocusLost(juce::TextEditor &textEditor) override;

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

    bool keyPressed(const juce::KeyPress& key,
        Component* originatingComponent);

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


    //Header
    juce::Label appTitle{ "appTitle" ,"L O O P S P A C E"};
    // Global controls
    //juce::TextButton playButton{ "Play" };
    //juce::TextButton stopButton{ "Stop" };
    TransportButton stopButton{ "stopButton",MAIN_BACKGROUND_COLOR,MAIN_BACKGROUND_COLOR,MAIN_BACKGROUND_COLOR, TransportButton::TransportButtonRole::Stop };
    TransportButton playButton{ "playButton",MAIN_BACKGROUND_COLOR,MAIN_BACKGROUND_COLOR,MAIN_BACKGROUND_COLOR, TransportButton::TransportButtonRole::Play };
    //juce::Label savedLoopsLabel{ "savedLoopLabel","Current Project File" };
    juce::ComboBox savedLoopsDropdown{ "savedLoopsDropdown" };
    DirectoryTree savedLoopDirTree;
    juce::TextButton saveButton{ "SAVE" };
    juce::TextButton initializeButton{ "NEW" };
    juce::AlertWindow saveProjectDialog{ "Save Project","Enter the name of your Loop Project:",juce::AlertWindow::AlertIconType::NoIcon };
    juce::AlertWindow unsavedProgressWarning{ "Unsaved Progress Warning","You will lose any unsaved progress.  Continue?",juce::AlertWindow::AlertIconType::WarningIcon };
    bool unsavedChanges = false; //DN: determines whether to warn about unsaved progress when switching projects
    int currentProjectListID = 0; //DN: keep track of where we are in the project list.  Update this when changing the dropdown

    CustomLookAndFeel customLookAndFeel;


    juce::OwnedArray<AudioTrack> tracksArray;

    juce::MixerAudioSource mixer;

    TransportState state;

    InputMonitor inputAudio;

    Metronome metronome;
    juce::TextButton metronomeButton{ "METRONOME" };

    juce::TextEditor tempoBox;
    juce::Label tempoBoxLabel;
    juce::TextEditor beatsBox;
    juce::Label beatsBoxLabel;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MainComponent)
};

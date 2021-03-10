#pragma once

#include "AudioTrack.h"
#include "InputMonitor.h"
#include "Metronome.h"
#include "BinaryData.h"



//==============================================================================
/*
    This component lives inside our window, and contains all
    controls and content.
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
    // AF: Text Box Listeners
    void textEditorReturnKeyPressed(juce::TextEditor &textEditor) override;
    void textEditorFocusLost(juce::TextEditor &textEditor) override;
    void textEditorTextChanged(juce::TextEditor& textEditor) override;

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
    void settingsButtonClicked();
    void metronomeButtonClicked();
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

    std::unique_ptr<juce::AudioDeviceSelectorComponent> audioSetupComp;
    juce::DialogWindow::LaunchOptions settingsWindow;

    //Header
    juce::Label appTitle{ "appTitle" ,"L O O P S P A C E"};


    // Global controls
    TransportButton stopButton{ "stopButton",MAIN_BACKGROUND_COLOR,MAIN_BACKGROUND_COLOR,MAIN_BACKGROUND_COLOR, TransportButton::TransportButtonRole::Stop };
    TransportButton playButton{ "playButton",MAIN_BACKGROUND_COLOR,MAIN_BACKGROUND_COLOR,MAIN_BACKGROUND_COLOR, TransportButton::TransportButtonRole::Play };
    
    juce::TextEditor tempoBox;
    juce::Label tempoBoxLabel;
    juce::TextEditor beatsBox;
    juce::Label beatsBoxLabel;

    Metronome metronome;
    std::unique_ptr<juce::Drawable> metronomeSVG;
    juce::DrawableButton metronomeButton{ "metronomeButton",juce::DrawableButton::ButtonStyle::ImageFitted };

    juce::ComboBox savedLoopsDropdown{ "savedLoopsDropdown" };
    DirectoryTree savedLoopDirTree;

    std::unique_ptr<juce::Drawable> saveSVG;
    juce::DrawableButton saveButton{ "saveButton",juce::DrawableButton::ButtonStyle::ImageFitted };
    NewFileButton initializeButton{ "initializeButton",MAIN_BACKGROUND_COLOR,MAIN_BACKGROUND_COLOR,MAIN_BACKGROUND_COLOR };
    juce::DrawableButton plusIcon{ "plusIcon",juce::DrawableButton::ButtonStyle::ImageFitted };
    std::unique_ptr<juce::Drawable> plusSVG;

    std::unique_ptr<juce::Drawable> settingsSVG;
    juce::DrawableButton settingsButton{ "settingsButton",juce::DrawableButton::ButtonStyle::ImageFitted };

    std::unique_ptr<juce::Drawable> loopLengthSVG;
    LoopLengthButton loopLengthButton{ "loopLengthButton",juce::DrawableButton::ButtonStyle::ImageFitted };


    // Dialog Windows
    juce::AlertWindow saveProjectDialog{ "Save Project","Enter the name of your Loop Project:",juce::AlertWindow::AlertIconType::NoIcon };
    juce::AlertWindow unsavedProgressWarning{ "Unsaved Progress Warning","You will lose any unsaved progress.  Continue?",juce::AlertWindow::AlertIconType::WarningIcon };
    
    // flags etc
    bool unsavedChanges = false; //DN: determines whether to warn about unsaved progress when switching projects
    int currentProjectListID = 0; //DN: keep track of where we are in the project list.  Update this when changing the dropdown
    bool settingsHaveBeenOpened = false; //DN: set to true once someone hits settings the first time

    //UI
    CustomLookAndFeel customLookAndFeel;
    SettingsLookAndFeel settingsLF;

    // Tracks / DSP
    juce::OwnedArray<AudioTrack> tracksArray;

    InputMonitor inputAudio;
    juce::MixerAudioSource mixer;

    TransportState state;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MainComponent)
};

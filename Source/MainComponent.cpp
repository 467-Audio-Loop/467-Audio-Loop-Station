#include "MainComponent.h"

const bool includeInput = true;

//==============================================================================
// AF: Constructor declaration for MainComponent()
MainComponent::MainComponent() : audioSetupComp(deviceManager,
                                                0,     // minimum input channels
                                                256,   // maximum input channels
                                                0,     // minimum output channels
                                                256,   // maximum output channels
                                                false, // ability to select midi inputs
                                                false, // ability to select midi output device
                                                false, // treat channels as stereo pairs
                                                false) // hide advanced options
{
    addAndMakeVisible(audioSetupComp);

    // AF: Initialize state enum
    state = Stopped;

    // AF: Adds record button and paints it
    addAndMakeVisible(track1RecordButton);
    track1RecordButton.setColour(juce::TextButton::buttonColourId, juce::Colour(0xffff5c5c));
    track1RecordButton.setColour(juce::TextButton::textColourOnId, juce::Colours::black);

    addAndMakeVisible(track2RecordButton);
    track2RecordButton.setColour(juce::TextButton::buttonColourId, juce::Colour(0xffff5c5c));
    track2RecordButton.setColour(juce::TextButton::textColourOnId, juce::Colours::black);

    addAndMakeVisible(track3RecordButton);
    track3RecordButton.setColour(juce::TextButton::buttonColourId, juce::Colour(0xffff5c5c));
    track3RecordButton.setColour(juce::TextButton::textColourOnId, juce::Colours::black);

    addAndMakeVisible(track4RecordButton);
    track4RecordButton.setColour(juce::TextButton::buttonColourId, juce::Colour(0xffff5c5c));
    track4RecordButton.setColour(juce::TextButton::textColourOnId, juce::Colours::black);

    // AF: Adds play button and paints it
    addAndMakeVisible(&playButton);
    playButton.onClick = [this] { playButtonClicked(); };
    playButton.setColour(juce::TextButton::buttonColourId, juce::Colours::green);
    playButton.setEnabled(true);

    // AF: Adds stop button and paints it
    addAndMakeVisible(&stopButton);
    stopButton.onClick = [this] { stopButtonClicked(); };
    stopButton.setColour(juce::TextButton::buttonColourId, juce::Colours::red);
    stopButton.setEnabled(false);

    //DN:  add saved loops label and dropdown menu
    addAndMakeVisible(&savedLoopsLabel);
    addAndMakeVisible(&savedLoopsDropdown);
    addAndMakeVisible(&saveButton);
    addAndMakeVisible(&initializeButton);

    saveButton.onClick = [this] { saveButtonClicked(); };
   // saveButton.setColour(juce::TextButton::buttonColourId, juce::Colours::green);
    saveButton.setEnabled(true);
    

    initializeButton.onClick = [this] { initializeButtonClicked(); };
    // initializeButton.setColour(juce::TextButton::buttonColourId, juce::Colours::green);
    initializeButton.setEnabled(true);


    //DN: Set up default directory loop wav files and feed them to Audio track objects
    refreshAudioReferences();

    //DN:  set up the dropdown that lets you load previously saved projects
    //DN: set first item index offset to 1, 0 will be when no project is selected
    savedLoopsDropdown.addItemList(savedLoopDirTree.getLoopFolderNamesArray(),1); 
    savedLoopsDropdown.setTextWhenNothingSelected("No Project Loaded");
    savedLoopsDropdown.setTextWhenNoChoicesAvailable("No Projects Found");
    savedLoopsDropdown.onChange = [this] { savedLoopSelected();  };



    track1.addChangeListener(this);
    track2.addChangeListener(this);
    track3.addChangeListener(this);
    track4.addChangeListener(this);

    addAndMakeVisible(track1);
    addAndMakeVisible(track2);
    addAndMakeVisible(track3);
    addAndMakeVisible(track4);

    mixer.addInputSource(&inputAudio, false);
    mixer.addInputSource(&track1, false);
    mixer.addInputSource(&track2, false);
    mixer.addInputSource(&track3, false);
    mixer.addInputSource(&track4, false);



    //We need to refactor a bit to DRY this section up
    track1RecordButton.onClick = [this]
    {
        

        if (track1.isRecording())
        {
            track1.stopRecording();

            inputAudio.setGain(0.0);  //DN: turn input monitoring off when going back to playback from recording

            //DN: we'll always be playing if recording so we don't need this anymore
            //    also calling start here will put things out of sync

            //// AF: Only enable play button if no tracks are currently playing.
            //if (state == Stopped)
            //{
            //    playButton.setEnabled(true);
            //}
            //else 
            //{
            //    track1.start(); 
            //}

            track1RecordButton.setButtonText("Record");

            // AF: Enable other track "Record" buttons
            track2RecordButton.setEnabled(true);
            track3RecordButton.setEnabled(true);
            track4RecordButton.setEnabled(true);

            track1.setDisplayFullThumbnail(true);
        }
        else
        {
            // AF: Begin playback when user clicks record if it's not already playing
            if (state == Stopped)
            {
                changeState(Starting);
            }

            inputAudio.setGain(1.0);  //DN: turn input monitoring on when recording

            // AF: Stop track from playing if this current track is actively playing,
            // before starting to record again over it
            //if (track1.isPlaying())
            //    track1.stop();


            if (!juce::RuntimePermissions::isGranted(juce::RuntimePermissions::writeExternalStorage))
            {
                SafePointer<MainComponent> safeThis(this);

                juce::RuntimePermissions::request(juce::RuntimePermissions::writeExternalStorage,
                    [safeThis](bool granted) mutable
                    {
                        if (granted)
                            safeThis->track1.startRecording();
                    });
                return;
            }

            track1RecordButton.setButtonText("Stop");

            //AF: Make other record buttons greyed out
            track2RecordButton.setEnabled(false);
            track3RecordButton.setEnabled(false);
            track4RecordButton.setEnabled(false);

            track1.startRecording();

            unsavedChanges = true; //if we record something we want to make sure to warn them to save it when switching projects
        }
    };

    track2RecordButton.onClick = [this]
    {
        if (track2.isRecording())
        {
            track2.stopRecording();

            inputAudio.setGain(0.0);  //DN: turn input monitoring off when going back to playback from recording

           
            //// AF: Only enable play button if no tracks are currently playing.
            //if (state == Stopped)
            //{
            //    playButton.setEnabled(true);
            //}
            //else // AF: If playback is happening, go ahead and start this track
            //{
            //    track2.start();
            //}


            track2RecordButton.setButtonText("Record");

            // AF: Enable other track "Record" buttons
            track1RecordButton.setEnabled(true);
            track3RecordButton.setEnabled(true);
            track4RecordButton.setEnabled(true);

            track2.setDisplayFullThumbnail(true);
        }
        else
        {
            // AF: Begin playback when user clicks record if it's not already playing
            if (state == Stopped)
            {
                changeState(Starting);
            }

            inputAudio.setGain(1.0);  //DN: turn input monitoring on when recording

            // AF: Stop track from playing if this current track is actively playing,
            // before starting to record again over it
            //if (track2.isPlaying())
            //    track2.stop();

            if (!juce::RuntimePermissions::isGranted(juce::RuntimePermissions::writeExternalStorage))
            {
                SafePointer<MainComponent> safeThis(this);

                juce::RuntimePermissions::request(juce::RuntimePermissions::writeExternalStorage,
                    [safeThis](bool granted) mutable
                    {
                        if (granted)
                            safeThis->track2.startRecording();
                    });
                return;
            }
            track2RecordButton.setButtonText("Stop");

            //AF: Make other record buttons greyed out
            track1RecordButton.setEnabled(false);
            track3RecordButton.setEnabled(false);
            track4RecordButton.setEnabled(false);

            track2.startRecording();

            unsavedChanges = true; //if we record something we want to make sure to warn them to save it when switching projects
        }
    };

    track3RecordButton.onClick = [this]
    {
        if (track3.isRecording())
        {
            track3.stopRecording();

            inputAudio.setGain(0.0);  //DN: turn input monitoring off when going back to playback from recording


            //// AF: Only enable play button if no tracks are currently playing.
            //if (state == Stopped)
            //{
            //    playButton.setEnabled(true);
            //}
            //else // AF: If playback is happening, go ahead and start this track
            //{
            //    track3.start();
            //}


            track3RecordButton.setButtonText("Record");

            // AF: Enable other track "Record" buttons
            track1RecordButton.setEnabled(true);
            track2RecordButton.setEnabled(true);
            track4RecordButton.setEnabled(true);

            track3.setDisplayFullThumbnail(true);
        }
        else
        {
            // AF: Begin playback when user clicks record if it's not already playing
            if (state == Stopped)
            {
                changeState(Starting);
            }

            inputAudio.setGain(1.0);  //DN: turn input monitoring on when recording

            // AF: Stop track from playing if this current track is actively playing,
            // before starting to record again over it
            //if (track3.isPlaying())
            //    track3.stop();

            if (!juce::RuntimePermissions::isGranted(juce::RuntimePermissions::writeExternalStorage))
            {
                SafePointer<MainComponent> safeThis(this);

                juce::RuntimePermissions::request(juce::RuntimePermissions::writeExternalStorage,
                    [safeThis](bool granted) mutable
                    {
                        if (granted)
                            safeThis->track3.startRecording();
                    });
                return;
            }
            track3RecordButton.setButtonText("Stop");

            //AF: Make other record buttons greyed out
            track1RecordButton.setEnabled(false);
            track2RecordButton.setEnabled(false);
            track4RecordButton.setEnabled(false);

            track3.startRecording();

            unsavedChanges = true; //if we record something we want to make sure to warn them to save it when switching projects
        }
    };

    track4RecordButton.onClick = [this]
    {
        if (track4.isRecording())
        {
            track4.stopRecording();

            inputAudio.setGain(0.0);  //DN: turn input monitoring off when going back to playback from recording

            //// AF: Only enable play button if no tracks are currently playing.
            //if (state == Stopped)
            //{
            //    playButton.setEnabled(true);
            //}
            //else // AF: If playback is happening, go ahead and start this track
            //{
            //    track4.start();
            //}

            track4RecordButton.setButtonText("Record");

            // AF: Enable other track "Record" buttons
            track1RecordButton.setEnabled(true);
            track2RecordButton.setEnabled(true);
            track3RecordButton.setEnabled(true);

            track4.setDisplayFullThumbnail(true);
        }
        else
        {
            // AF: Begin playback when user clicks record if it's not already playing
            if (state == Stopped)
            {
                changeState(Starting);
            }

            inputAudio.setGain(1.0);  //DN: turn input monitoring on when recording

            // AF: Stop track from playing if this current track is actively playing,
            // before starting to record again over it
            //if (track4.isPlaying())
             //   track4.stop();

            if (!juce::RuntimePermissions::isGranted(juce::RuntimePermissions::writeExternalStorage))
            {
                SafePointer<MainComponent> safeThis(this);

                juce::RuntimePermissions::request(juce::RuntimePermissions::writeExternalStorage,
                    [safeThis](bool granted) mutable
                    {
                        if (granted)
                            safeThis->track4.startRecording();
                    });
                return;
            }
            track4RecordButton.setButtonText("Stop");

            //AF: Make other record buttons greyed out
            track1RecordButton.setEnabled(false);
            track2RecordButton.setEnabled(false);
            track3RecordButton.setEnabled(false);

            track4.startRecording();

            unsavedChanges = true; //if we record something we want to make sure to warn them to save it when switching projects
        }
    };


    // Some platforms require permissions to open input channels so request that here
    if (juce::RuntimePermissions::isRequired (juce::RuntimePermissions::recordAudio)
        && ! juce::RuntimePermissions::isGranted (juce::RuntimePermissions::recordAudio))
    {
        juce::RuntimePermissions::request (juce::RuntimePermissions::recordAudio,
                                           [&] (bool granted) { setAudioChannels (granted ? 2 : 0, 2); });
    }
    else
    {
        // Specify the number of input and output channels that we want to open
        setAudioChannels (2, 2);
    }

    
    deviceManager.addAudioCallback(&track1);
    deviceManager.addAudioCallback(&track2);
    deviceManager.addAudioCallback(&track3);
    deviceManager.addAudioCallback(&track4);


    deviceManager.addChangeListener(this);  



    // Make sure you set the size of the component after
    // you add any child components.
    setSize(1000, 800);
}

MainComponent::~MainComponent()
{
    deviceManager.removeChangeListener(this);
    
    // This shuts down the audio device and clears the audio source.
    shutdownAudio();
}

//==============================================================================
void MainComponent::prepareToPlay (int samplesPerBlockExpected, double sampleRate)
{
    // This function will be called when the audio device is started, or when
    // its settings (i.e. sample rate, block size, etc) are changed.

    // You can use this function to initialise any resources you might need,
    // but be careful - it will be called on the audio thread, not the GUI thread.    

   /* track1.prepareToPlay(samplesPerBlockExpected, sampleRate);
    track2.prepareToPlay(samplesPerBlockExpected, sampleRate);
    track3.prepareToPlay(samplesPerBlockExpected, sampleRate);
    track4.prepareToPlay(samplesPerBlockExpected, sampleRate);*/
    mixer.prepareToPlay(samplesPerBlockExpected, sampleRate);
}

void MainComponent::getNextAudioBlock (const juce::AudioSourceChannelInfo& bufferToFill)
{
   // bufferToFill.clearActiveBufferRegion();  //DN: start with silence, so if we need it it's already there

    auto* device = deviceManager.getCurrentAudioDevice();
    auto activeInputChannels = device->getActiveInputChannels();
    auto activeOutputChannels = device->getActiveOutputChannels();
    auto maxInputChannels = activeInputChannels.getHighestBit() + 1;
    auto maxOutputChannels = activeOutputChannels.getHighestBit() + 1;

    auto sourceBuffer = std::make_unique<juce::AudioBuffer<float>>(maxInputChannels, bufferToFill.numSamples);

    /// DN: This code grabs the audio input, puts it in a buffer, and sends that to an AudioSource class
    //  which can be added to or removed from our main mixer as needed
  
    for (auto channel = 0; channel < maxOutputChannels; ++channel)
    {
        if ((!activeOutputChannels[channel]) || maxInputChannels == 0)
        {
            bufferToFill.buffer->clear(channel, bufferToFill.startSample, bufferToFill.numSamples);
        }
        else
        {
            auto actualInputChannel = channel % maxInputChannels;

            if (!activeInputChannels[channel])
            {
                bufferToFill.buffer->clear(channel, bufferToFill.startSample, bufferToFill.numSamples);
            }
            else
            {
                //DN: get the input and fill the correct channel of our source buffer
                auto* reader = bufferToFill.buffer->getReadPointer(actualInputChannel,
                    bufferToFill.startSample);

                auto* writer = sourceBuffer->getWritePointer(channel % maxInputChannels, bufferToFill.startSample);

                for (auto sample = 0; sample < bufferToFill.numSamples; ++sample)
                    writer[sample] = reader[sample];

            }
        }
    }
    //send filled buffer to the AudioSource
    inputAudio.setBuffer(sourceBuffer.release());



    //DN: We've added the tracks to the mixer already, so this will trigger all of them
    mixer.getNextAudioBlock(bufferToFill);
}

void MainComponent::releaseResources()
{
    // This will be called when the audio device stops, or when it is being
    // restarted due to a setting change.

    // For more details, see the help for AudioProcessor::releaseResources()
    track1.releaseResources();
    track2.releaseResources();
    track3.releaseResources();
    track4.releaseResources();
    mixer.releaseResources();
}

//==============================================================================
void MainComponent::paint (juce::Graphics& g)
{
    // (Our component is opaque, so we must completely fill the background with a solid colour)
    // g.fillAll (getLookAndFeel().findColour (juce::ResizableWindow::backgroundColourId));

    // You can add your drawing code here!
}

void MainComponent::resized()
{
    // This is called when the MainContentComponent is resized.
    // If you add any child components, this is where you should
    // update their positions.
    auto rect = getLocalBounds().reduced(10);
    auto globalControlsArea = rect.removeFromTop(400);

    audioSetupComp.setBounds(globalControlsArea.removeFromLeft(proportionOfWidth(0.5f)));

    auto transportControlsArea = globalControlsArea.removeFromTop(80);
    transportControlsArea.reduce(50, 0);
    stopButton.setBounds(transportControlsArea.removeFromLeft(transportControlsArea.proportionOfWidth(0.5f)).reduced(10));
    playButton.setBounds(transportControlsArea.reduced(10));

    auto gapBetweenStuff = globalControlsArea.removeFromTop(50);

    auto savedLoopsArea = globalControlsArea.removeFromTop(150);
    savedLoopsLabel.setBounds(savedLoopsArea.removeFromTop(50));
    savedLoopsLabel.setJustificationType(juce::Justification::centredBottom);
    savedLoopsDropdown.setBounds(savedLoopsArea.removeFromTop(50).reduced(5));
    auto saveClearButtonsArea = (savedLoopsArea.removeFromTop(50));
    saveButton.setBounds(saveClearButtonsArea.removeFromLeft(proportionOfWidth(0.25f)).reduced(10));
    initializeButton.setBounds(saveClearButtonsArea.reduced(10));




    

    
    auto track1Area = rect.removeFromTop(80);
    track1RecordButton.setBounds(track1Area.removeFromLeft(140).reduced(10));
    track1.setBounds(track1Area.reduced(8));
    auto track2Area = rect.removeFromTop(80);
    track2RecordButton.setBounds(track2Area.removeFromLeft(140).reduced(10));
    track2.setBounds(track2Area.reduced(8));
    auto track3Area = rect.removeFromTop(80);
    track3RecordButton.setBounds(track3Area.removeFromLeft(140).reduced(10));
    track3.setBounds(track3Area.reduced(8));
    auto track4Area = rect.removeFromTop(80);
    track4RecordButton.setBounds(track4Area.removeFromLeft(140).reduced(10));
    track4.setBounds(track4Area.reduced(8));




}


// AF: ========================= New Audio Playing Declarations ================================

void MainComponent::changeListenerCallback(juce::ChangeBroadcaster* source)
{
    if (source == &track1 || source == &track2 || source == &track3 || source == &track4)
    {
        //DN:  we need this to handle things if recording is cut off automatically
        if (!trackCurrentlyRecording())
        {
            track1RecordButton.setButtonText("Record");
            track2RecordButton.setButtonText("Record");
            track3RecordButton.setButtonText("Record");
            track4RecordButton.setButtonText("Record");

            track1RecordButton.setEnabled(true);
            track2RecordButton.setEnabled(true);
            track3RecordButton.setEnabled(true);
            track4RecordButton.setEnabled(true);

            track1.setDisplayFullThumbnail(true);
            track2.setDisplayFullThumbnail(true);
            track3.setDisplayFullThumbnail(true);
            track4.setDisplayFullThumbnail(true);

            inputAudio.setGain(0.0);
        }
    }
    if (source == &track1)
    {
        if (track1.isPlaying())
            changeState(Playing);
        else
            changeState(Stopped);

    }

    if (source == &track2)
    {
            if (track2.isPlaying())
                changeState(Playing);
            else
                changeState(Stopped);
    }

    if (source == &track3)
    {
            if (track3.isPlaying())
                changeState(Playing);
            else
                changeState(Stopped);
    }

    if (source == &track4)
    {
            if (track4.isPlaying())
                changeState(Playing);
            else
                changeState(Stopped);
    }
}

void MainComponent::playButtonClicked()
{
    changeState(Starting);
    inputAudio.setGain(0.0);
}

void MainComponent::stopButtonClicked()
{
    changeState(Stopping);
    inputAudio.setGain(1.0);
}

void MainComponent::saveButtonClicked()
{
    juce::AlertWindow saveProjectDialog{ "Save Project","Enter the name of your Loop Project:",juce::AlertWindow::AlertIconType::NoIcon };

    //sets up the alert window for when you hit Save
    saveProjectDialog.addTextEditor("newProjectName", "");
    saveProjectDialog.addButton("Cancel", 0);
    saveProjectDialog.addButton("Save", 1);
    
    juce::String newFolderName;

    //DN: if this project is new, we need to name it/create a folder for it
    if (savedLoopsDropdown.getSelectedId() == 0)
    {
        auto result = saveProjectDialog.runModalLoop();
        newFolderName = saveProjectDialog.getTextEditorContents("newProjectName");
        if (newFolderName.length() == 0)
            saveProjectDialog.addTextBlock("New Project Name Cannot Be Empty");
        while (newFolderName.length() == 0)
        {
            result = saveProjectDialog.runModalLoop();
            newFolderName = saveProjectDialog.getTextEditorContents("newProjectName");
        }
        saveProjectDialog.setVisible(false);

        savedLoopDirTree.saveWAVsTo(newFolderName);


    }
    else
    {
        savedLoopDirTree.saveWAVsTo(savedLoopsDropdown.getText());  //DN: save loop to project folder selected in dropdown
    }

    //DN: now we refresh the dropdown list with the current folders
    //and find the folder we just saved To, then make it the current selection
    juce::StringArray folderNames = savedLoopDirTree.getLoopFolderNamesArray();
    savedLoopsDropdown.clear();
    savedLoopsDropdown.addItemList(folderNames,1); //DN: set first item index offset to 1, 0 will be when no project is selected
    for (int i = 0; i < folderNames.size(); ++i)
    {
        if (folderNames[i] == newFolderName)
            savedLoopsDropdown.setSelectedId(i + 1,juce::dontSendNotification); //account for dropdown index offset, don't trigger savedLoopSelected
    }
    unsavedChanges = false;

}

void MainComponent::initializeButtonClicked()
{
    auto result = unsavedProgressWarning.runModalLoop();
}

void MainComponent::savedLoopSelected()
{

    //creates a warning here that will reset the dropdown and abort this function if they hit Cancel
    if (savedLoopsDropdown.getSelectedId() != 0 && unsavedChanges)
    {
        auto okCancelSelection = unsavedProgressWarning.showOkCancelBox(juce::AlertWindow::AlertIconType::WarningIcon,"Unsaved Progress Warning",
            "You will lose any unsaved progress.  Continue?",
            "Ok", "Cancel", nullptr);
        if (okCancelSelection == 0)
        {
            savedLoopsDropdown.setSelectedId(0);
            return;
        }
    }
    

    //if they hit ok, then go ahead and load the selection
    juce::String savedLoopFolderName = savedLoopsDropdown.getText();

    bool test = savedLoopDirTree.loadWAVsFrom(savedLoopFolderName);

    //DN: only try to read files if copying them was successfull
    if (test)
    {
        refreshAudioReferences();
        redrawAndBufferAudio();
    }

    unsavedChanges = false;

}

void MainComponent::refreshAudioReferences()
{
    auto track1File = savedLoopDirTree.getOrCreateWAVInTempLoopDir(TRACK1_FILENAME);
    auto track2File = savedLoopDirTree.getOrCreateWAVInTempLoopDir(TRACK2_FILENAME);
    auto track3File = savedLoopDirTree.getOrCreateWAVInTempLoopDir(TRACK3_FILENAME);
    auto track4File = savedLoopDirTree.getOrCreateWAVInTempLoopDir(TRACK4_FILENAME);
    track1.setLastRecording(track1File);
    track2.setLastRecording(track2File);
    track3.setLastRecording(track3File);
    track4.setLastRecording(track4File);
}

//DN:  call after refreshAudioReferences to load the audio into memory and redraw waveforms
void MainComponent::redrawAndBufferAudio()
{
    track1.redrawAndBufferAudio();
    track2.redrawAndBufferAudio();
    track3.redrawAndBufferAudio();
    track4.redrawAndBufferAudio();

}

bool MainComponent::trackCurrentlyPlaying()
{
    if (track1.isPlaying() || track2.isPlaying() || track3.isPlaying() || track4.isPlaying())
    {
        return true;
    }

    return false;
}

bool MainComponent::trackCurrentlyRecording()
{
    if (track1.isRecording() || track2.isRecording() || track3.isRecording() || track4.isRecording())
    {
        return true;
    }

    return false;
}

void MainComponent::changeState(TransportState newState)
{
    if (state != newState)
    {
        state = newState;

        switch (state)
        {
        case Stopped:                           
            stopButton.setEnabled(false);
            playButton.setEnabled(true);
            savedLoopsDropdown.setEnabled(true);
            saveButton.setEnabled(true);
            initializeButton.setEnabled(true);
            track1.setPosition(0);
            track2.setPosition(0);
            track3.setPosition(0);
            track4.setPosition(0);
            break;

        case Starting: 
            playButton.setEnabled(false);
            savedLoopsDropdown.setEnabled(false);
            saveButton.setEnabled(false);
            initializeButton.setEnabled(false);
            track1.start();
            track2.start();
            track3.start();
            track4.start();
            break;

        case Playing:                           
            stopButton.setEnabled(true);
            break;

        case Stopping:                          
            track1.stop();
            track2.stop();
            track3.stop();
            track4.stop();
            break;

        }
    }
}
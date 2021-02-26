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

    //DN: create tracks 
    for (int i = 0; i < NUM_TRACKS; ++i)
    {
        auto track = new AudioTrack;
        tracksArray.add(track);
    }


    for (auto& track : tracksArray)
    {
        addAndMakeVisible(track->panSlider);
        track->panSlider.setNumDecimalPlacesToDisplay(2);
        track->panSlider.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 40, 20);
        addAndMakeVisible(track->panLabel);

        // DN: Show reverse buttons and slip controllers
        addAndMakeVisible(track->reverseButton);
        addAndMakeVisible(track->slipController);
        track->slipController.setNumDecimalPlacesToDisplay(2);
        track->slipController.setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);

        // AF: Adds record button and paints it
        addAndMakeVisible(track->recordButton);
        track->recordButton.setColour(juce::TextButton::buttonColourId, juce::Colour(0xffff5c5c));
        track->recordButton.setColour(juce::TextButton::textColourOnId, juce::Colours::black);

        track->addChangeListener(this);

        addAndMakeVisible(*track);
        mixer.addInputSource(track, false);

        deviceManager.addAudioCallback(track);

        //callback lambda for each track's record button
        track->recordButton.onClick = [this, &track]
        {


            if (track->isRecording())
            {
                track->stopRecording();

                //inputAudio.setGain(0.0);  //DN: turn input monitoring off when going back to playback from recording


                track->recordButton.setButtonText("Record");

                // AF: Enable other track "Record" buttons
                for (auto& otherTrack : tracksArray)
                {
                    if (&otherTrack != &track)
                        otherTrack->recordButton.setEnabled(true);
                }               

                track->setDisplayFullThumbnail(true);
            }
            else
            {
                // AF: Begin playback when user clicks record if it's not already playing
                if (state == Stopped)
                {
                    changeState(Starting);
                }

                //inputAudio.setGain(1.0);  //DN: turn input monitoring on when recording

                if (!juce::RuntimePermissions::isGranted(juce::RuntimePermissions::writeExternalStorage))
                {
                    SafePointer<MainComponent> safeThis(this);

                    juce::RuntimePermissions::request(juce::RuntimePermissions::writeExternalStorage,
                        [safeThis, &track](bool granted) mutable
                        {
                            if (granted)
                            {
                                for (auto& i : safeThis->tracksArray)
                                {
                                    if (&i != &track)
                                        i->startRecording();
                                }
                            }                               
                        });
                    return;
                }

                track->recordButton.setButtonText("Stop");

                //AF: Make other record buttons greyed out
                // AF: Enable other track "Record" buttons
                for (auto& otherTrack : tracksArray)
                {
                    if (&otherTrack != &track)
                        otherTrack->recordButton.setEnabled(false);
                }

                track->startRecording();

                unsavedChanges = true; //if we record something we want to make sure to warn them to save it when switching projects
            }
        };
    }

    mixer.addInputSource(&inputAudio, false);

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
    initializeTempWAVs();

    //DN:  set up the dropdown that lets you load previously saved projects
    //DN: set first item index offset to 1, 0 will be when no project is selected
    savedLoopsDropdown.addItemList(savedLoopDirTree.getLoopFolderNamesArray(),1); 
    savedLoopsDropdown.setTextWhenNothingSelected("No Project Loaded");
    savedLoopsDropdown.setTextWhenNoChoicesAvailable("No Projects Found");
    savedLoopsDropdown.onChange = [this] { savedLoopSelected();  };



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

   
    deviceManager.addChangeListener(this);  

    unsavedProgressWarning.addKeyListener(this);
    saveProjectDialog.addKeyListener(this);  //DN:these are so we can hit Enter on the alert windows


    //sets up the alert window for when you hit Save
    saveProjectDialog.addTextEditor("newProjectName", "");
    saveProjectDialog.addButton("Cancel", 0);
    saveProjectDialog.addButton("Save", 1);


    // Make sure you set the size of the component after
    // you add any child components.
    setSize(1200, 900);
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
    auto maxInputChannels = activeInputChannels.countNumberOfSetBits();
    auto maxOutputChannels = activeOutputChannels.countNumberOfSetBits();
    auto test = device->getInputChannelNames();

    auto sourceBuffer = std::make_unique<juce::AudioBuffer<float>>(maxInputChannels, bufferToFill.numSamples);

    /// DN: This code grabs the audio input, puts it in a buffer, and sends that to an AudioSource class
    //  which can be added to or removed from our main mixer as needed
  
    for (auto channel = 0; channel < maxOutputChannels; ++channel)
    {
        if (maxInputChannels == 0)
        {
            bufferToFill.buffer->clear(channel, bufferToFill.startSample, bufferToFill.numSamples);
            sourceBuffer->clear();
        }
        else
        {
            auto actualInputChannel = channel % maxInputChannels;

            //DN: get the input and fill the correct channel of our source buffer
            auto* reader = bufferToFill.buffer->getReadPointer(actualInputChannel,
                bufferToFill.startSample);

            auto* writer = sourceBuffer->getWritePointer(channel % maxInputChannels, bufferToFill.startSample);

            for (auto sample = 0; sample < bufferToFill.numSamples; ++sample)
                writer[sample] = reader[sample];

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




    for (auto& track : tracksArray)
    {
        auto trackArea = rect.removeFromTop(120);
        auto trackControlsL = trackArea.removeFromLeft(140);
        track->recordButton.setBounds(trackControlsL.removeFromTop(60).reduced(6));
        track->panSlider.setBounds(trackControlsL);
        auto trackControlsR = trackArea.removeFromLeft(80);
        trackControlsR.reduce(0, 40);
        track->reverseButton.setBounds(trackControlsR);
        track->slipController.setBounds(trackArea.removeFromBottom(20));
        track->setBounds(trackArea.reduced(8));
    }

    


}


// AF: ========================= New Audio Playing Declarations ================================

void MainComponent::changeListenerCallback(juce::ChangeBroadcaster* source)
{
    for (auto& track : tracksArray)
    {
        if (source == track)
        {
            //DN:  we need this to handle things if recording is cut off automatically
            if (!trackCurrentlyRecording())
            {
                for (auto& i : tracksArray)
                {
                    i->setDisplayFullThumbnail(true);
                    i->recordButton.setButtonText("Record");
                    i->recordButton.setEnabled(true);
                }

            }

            if (track->isPlaying())
                changeState(Playing);
            else
                changeState(Stopped);
        }
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

    // AF: Stop tracks if stop button is clicked
    for (auto& track : tracksArray)
    {
        if (track->isRecording())
        {
            track->stopRecording();
        }
    }
}

void MainComponent::saveButtonClicked()
{
    juce::String newFolderName;

    //DN: if this project is new, we need to name it/create a folder for it
    if (savedLoopsDropdown.getSelectedId() == 0)
    {
        auto result = saveProjectDialog.runModalLoop();
        if (result == 1)
        {
            newFolderName = saveProjectDialog.getTextEditorContents("newProjectName");
            if (newFolderName.length() == 0)
                saveProjectDialog.addTextBlock("New Project Name Cannot Be Empty");
            while (newFolderName.length() == 0)
            {
                result = saveProjectDialog.runModalLoop();
                newFolderName = saveProjectDialog.getTextEditorContents("newProjectName");
            }
            savedLoopDirTree.saveWAVsTo(newFolderName);
        }

        saveProjectDialog.setVisible(false);
        //DN: the dialog box just goes behind everything until it gets called to the front again
        //    so we need to clear the text box for next time
        auto textEditor = saveProjectDialog.getTextEditor("newProjectName");
        textEditor->setText("");
    }
    else
    {
        newFolderName = savedLoopsDropdown.getText();
        savedLoopDirTree.saveWAVsTo(newFolderName);  //DN: save loop to project folder selected in dropdown
    }

    //DN: now we refresh the dropdown list with the current folders
    //and find the folder we just saved To, then make it the current selection
    juce::StringArray folderNames = savedLoopDirTree.getLoopFolderNamesArray();
    savedLoopsDropdown.clear();
    savedLoopsDropdown.addItemList(folderNames,1); //DN: set first item index offset to 1, 0 will be when no project is selected
    for (int i = 0; i < folderNames.size(); ++i)
    {
        if (folderNames[i] == newFolderName)
        {
            savedLoopsDropdown.setSelectedId(i + 1, juce::dontSendNotification); //account for dropdown index offset, don't trigger savedLoopSelected
            currentProjectListID = i+1;
        }
            
    }


    //DN: save the track states to an xml file in the project folder

    // create an outer node
    juce::XmlElement projectState("projectState");

    for (int i = 0; i < NUM_TRACKS; ++i)
    {
        projectState.addChildElement(tracksArray[i]->getTrackState(i+1));
    }

    //write it to a file in this project's folder
    juce::File destFile = savedLoopDirTree.getProjectFolder(newFolderName).getChildFile(PROJECT_STATE_XML_FILENAME);
    
    projectState.writeTo(destFile);



    unsavedChanges = false;
}

void MainComponent::initializeButtonClicked()
{
    //creates a warning here that will abort this function if they hit Cancel
    if (unsavedChanges)
    {
        auto okCancelSelection = unsavedProgressWarning.showOkCancelBox(juce::AlertWindow::AlertIconType::WarningIcon, "Unsaved Progress Warning",
            "You will lose any unsaved progress.  Continue?",
            "Ok", "Cancel", nullptr);
        if (okCancelSelection == 0)
        {
            return;
        }
    }

    savedLoopsDropdown.setSelectedId(0,juce::dontSendNotification);
    currentProjectListID = 0;
    initializeTempWAVs();
    redrawAndBufferAudio();
    for (auto& track : tracksArray)
    {
        track->initializeTrackState();
    }
    unsavedChanges = false;
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
            savedLoopsDropdown.setSelectedId(currentProjectListID,juce::dontSendNotification);
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

    //load xml from project folder
    auto projectFolder = savedLoopDirTree.getProjectFolder(savedLoopFolderName);
    juce::XmlDocument projectStateDoc(juce::File(projectFolder.getChildFile(PROJECT_STATE_XML_FILENAME)));

    //DN: can only try to acces the result of this if the file exists
    if (auto projectState = projectStateDoc.getDocumentElement())
    {
        //iterate through xml and restore the state of each track
        for (int i = 0; i < NUM_TRACKS; ++i)
        {
            forEachXmlChildElement(*projectState, trackState)
            {
                juce::String trackName = TRACK_FILENAME + juce::String(i + 1);
                if (trackState->hasTagName(trackName))
                    tracksArray[i]->restoreTrackState(trackState);
            }
        }
    }

    currentProjectListID = savedLoopsDropdown.getSelectedId();
    unsavedChanges = false;

}

bool MainComponent::keyPressed(const juce::KeyPress& key, juce::Component* originatingComponent)
{
    if (originatingComponent == &unsavedProgressWarning || originatingComponent == &saveProjectDialog)
        originatingComponent->exitModalState(1);  //DN: if someone hits enter on an Alertwindow, it's the same as clicking OK
    return true;
}

void MainComponent::initializeTempWAVs()
{
    for (int i = 0; i < NUM_TRACKS; ++i)
    {
        juce::String fileName = TRACK_FILENAME + juce::String(i + 1);
        auto trackFile = savedLoopDirTree.setFreshWAVInTempLoopDir(fileName);
        tracksArray[i]->setLastRecording(trackFile);
    }
}

void MainComponent::refreshAudioReferences()
{
    for (int i = 0; i < NUM_TRACKS; ++i)
    {
        juce::String fileName = TRACK_FILENAME + juce::String(i + 1);
        auto trackFile = savedLoopDirTree.getOrCreateWAVInTempLoopDir(fileName);
        tracksArray[i]->setLastRecording(trackFile);
    }
}

//DN:  call after refreshAudioReferences to load the audio into memory and redraw waveforms
void MainComponent::redrawAndBufferAudio()
{
    for (auto& track : tracksArray)
    {
        track->redrawAndBufferAudio();
    }

}

bool MainComponent::trackCurrentlyPlaying()
{
    for (auto& track : tracksArray)
    {
        if (track->isPlaying())
            return true;
    }
    return false;
}

bool MainComponent::trackCurrentlyRecording()
{
    for (auto& track : tracksArray)
    {
        if (track->isRecording())
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
            for (auto& track : tracksArray)
            {
                track->setPosition(0);
            }
            break;

        case Starting: 
            playButton.setEnabled(false);
            savedLoopsDropdown.setEnabled(false);
            saveButton.setEnabled(false);
            initializeButton.setEnabled(false);
            for (auto& track : tracksArray)
            {
                track->start();
            }
            break;

        case Playing:                           
            stopButton.setEnabled(true);
            break;

        case Stopping:       
            for (auto& track : tracksArray)
            {
                track->stop();
            }
            break;

        }
    }
}
#include "MainComponent.h"

const bool includeInput = true;

//==============================================================================
// AF: Constructor declaration for MainComponent()
MainComponent::MainComponent()
{
    //DN:: set up audio settings menu
    audioSetupComp = std::make_unique<juce::AudioDeviceSelectorComponent>(
                                        deviceManager,
                                        0,     // minimum input channels
                                        256,   // maximum input channels
                                        0,     // minimum output channels
                                        256,   // maximum output channels
                                        false, // ability to select midi inputs
                                        false, // ability to select midi output device
                                        false, // treat channels as stereo pairs
                                        false);
    audioSetupComp->setLookAndFeel(&settingsLF);


    // AF: Initialize state enum
    state = Stopped;

    getTopLevelComponent()->addKeyListener(this);

    setLookAndFeel(&customLookAndFeel);

    // AF: Tempo and beats control
    tempoBox.setFont(EDITOR_FONT);
    addAndMakeVisible(&tempoBox);
    tempoBox.setText("120");
    tempoBox.setInputRestrictions(3, "0123456789");
    tempoBox.setJustification(juce::Justification::centred);
    tempoBox.setSelectAllWhenFocused(true);
    tempoBox.addListener(this);
    addAndMakeVisible(&tempoBoxLabel);
    tempoBoxLabel.setFont(LABEL_FONT);
    tempoBoxLabel.setText("TEMPO", juce::dontSendNotification);
    tempoBoxLabel.setJustificationType(juce::Justification::centred);
    tempoBoxLabel.attachToComponent(&tempoBox, false);
    beatsBox.setFont(EDITOR_FONT);
    addAndMakeVisible(&beatsBox);
    beatsBox.setText("8");
    beatsBox.setJustification(juce::Justification::centred);
    beatsBox.setInputRestrictions(2, "0123456789");
    beatsBox.setSelectAllWhenFocused(true);
    beatsBox.addListener(this);
    addAndMakeVisible(&beatsBoxLabel);
    beatsBoxLabel.setText("BEATS", juce::dontSendNotification);
    beatsBoxLabel.setJustificationType(juce::Justification::centred);
    beatsBoxLabel.attachToComponent(&beatsBox, false);
    beatsBoxLabel.setFont(LABEL_FONT);


    //tell the loopLength drag controller button about beatsBox
    auto boxPtr = &beatsBox;
    loopLengthButton.setBeatsBox(boxPtr);

    // AF: Metronome
    mixer.addInputSource(&metronome, false);
    addAndMakeVisible(&metronomeButton);
    metronome.setBpm(tempoBox.getText().getIntValue());
    metronomeButton.onClick = [this] { metronomeButtonClicked(); };

    //DN: create tracks 
    for (int i = 0; i < NUM_TRACKS; ++i)
    {
        auto track = new AudioTrack;
        tracksArray.add(track);
    }


    //Initialize all tracks
    for (auto& track : tracksArray)
    {
        track->setMasterLoop(tempoBox.getText().getIntValue(), beatsBox.getText().getIntValue());
        addAndMakeVisible(track->panSlider);

        addAndMakeVisible(track->gainSlider);
        track->gainSlider.setNumDecimalPlacesToDisplay(2);

        //DN: set up reverse icon
        std::unique_ptr<juce::XmlElement> reverse_svg_xml(juce::XmlDocument::parse(BinaryData::fadrepeat_svg)); // GET THE SVG AS A XML
        track->reverseSVG = juce::Drawable::createFromSVG(*reverse_svg_xml.get()); // GET THIS AS DRAWABLE
        track->reverseButton.setImages(track->reverseSVG.get());

        addAndMakeVisible(track->reverseButton);
        addAndMakeVisible(track->recordButton);
        track->recordButton.setColour(juce::TextButton::textColourOnId, juce::Colours::black);
        track->addChangeListener(this);
        addAndMakeVisible(*track);
        mixer.addInputSource(track, false);

        deviceManager.addAudioCallback(track);

        //callback lambda for each track's record button
        track->recordButton.onClick = [this, &track]
        {
            tempoBox.setReadOnly(true);
            tempoBox.setEnabled(false);
            tempoBox.setColour(juce::TextEditor::textColourId, SECONDARY_DRAW_COLOR);
            auto text = tempoBox.getText();
            tempoBox.clear();
            tempoBox.setText(text);
            tempoBoxLabel.setColour(juce::Label::textColourId, SECONDARY_DRAW_COLOR);

            if (track->isRecording())
            {
                track->stopRecording();

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

                //DN: this prevents the exception that happens if you don't have any audio card set up
                if (deviceManager.getCurrentAudioDevice())
                {
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

                    //AF: Make other record buttons disabled
                    for (auto& otherTrack : tracksArray)
                    {
                        if (&otherTrack != &track)
                            otherTrack->recordButton.setEnabled(false);
                    }
                    
                    track->setWaitingToRecord(true);
                    unsavedChanges = true; //if we record something we want to make sure to warn them to save it when switching projects
                }
            }
        };
    }

    //DN: Set up default directory loop wav files and feed them to Audio track objects
    initializeTempWAVs();
    redrawAndBufferAudio();

    mixer.addInputSource(&inputAudio, false);

    addAndMakeVisible(&appTitle);
    appTitle.setJustificationType(juce::Justification::centred);
    appTitle.setFont(customLookAndFeel.titleFont);

    //DN: set up icons
    std::unique_ptr<juce::XmlElement> settings_svg_xml(juce::XmlDocument::parse(BinaryData::cogsolid_svg)); // GET THE SVG AS A XML
    settingsSVG = juce::Drawable::createFromSVG(*settings_svg_xml.get()); // GET THIS AS DRAWABLE
    settingsButton.setImages(settingsSVG.get());

    std::unique_ptr<juce::XmlElement> save_svg_xml(juce::XmlDocument::parse(BinaryData::fadsave_svg)); // GET THE SVG AS A XML
    saveSVG = juce::Drawable::createFromSVG(*save_svg_xml.get()); // GET THIS AS DRAWABLE
    saveButton.setImages(saveSVG.get());

    std::unique_ptr<juce::XmlElement> plus_svg_xml(juce::XmlDocument::parse(BinaryData::plussolid_svg)); // GET THE SVG AS A XML
    plusSVG = juce::Drawable::createFromSVG(*plus_svg_xml.get()); // GET THIS AS DRAWABLE
    plusIcon.setImages(plusSVG.get());

    std::unique_ptr<juce::XmlElement> metronome_svg_xml(juce::XmlDocument::parse(BinaryData::fadmetronome_svg)); // GET THE SVG AS A XML
    metronomeSVG = juce::Drawable::createFromSVG(*metronome_svg_xml.get()); // GET THIS AS DRAWABLE
    metronomeButton.setImages(metronomeSVG.get());

    std::unique_ptr<juce::XmlElement> loopLength_svg_xml(juce::XmlDocument::parse(BinaryData::linewarrows_svg)); // GET THE SVG AS A XML
    loopLengthSVG = juce::Drawable::createFromSVG(*loopLength_svg_xml.get()); // GET THIS AS DRAWABLE
    loopLengthButton.setImages(loopLengthSVG.get());


    // AF: Adds play button and paints it
    addAndMakeVisible(&playButton);
    playButton.onClick = [this] { playButtonClicked(); };
    //playButton.setColour(juce::TextButton::buttonColourId, juce::Colours::green);
    playButton.setEnabled(true);

    // AF: Adds stop button and paints it

    addAndMakeVisible(&stopButton);
    stopButton.onClick = [this] { stopButtonClicked(); };
    stopButton.setEnabled(false);

    //DN:  add saved loops label and dropdown menu
    addAndMakeVisible(&savedLoopsDropdown);
    addAndMakeVisible(&saveButton);
    addAndMakeVisible(&initializeButton);
    addAndMakeVisible(&plusIcon,-1);
    addAndMakeVisible(&settingsButton);

    saveButton.onClick = [this] { saveButtonClicked(); };
    saveButton.setEnabled(true);
    
    initializeButton.onClick = [this] { initializeButtonClicked(); };
    initializeButton.setEnabled(true);

    plusIcon.onClick = [this] {initializeButtonClicked(); }; //DN: plus icon is part of the init button
    plusIcon.setEnabled(true);

    settingsButton.onClick = [this] { settingsButtonClicked(); };
    settingsButton.setEnabled(true);

    addAndMakeVisible(&loopLengthButton);

    loopLengthButton.onClick = [this] {textEditorReturnKeyPressed(beatsBox); };

    //DN:  set up the dropdown that lets you load previously saved projects
    //DN: set first item index offset to 1, 0 will be when no project is selected
    savedLoopsDropdown.addItemList(savedLoopDirTree.getLoopFolderNamesArray(),1); 
    savedLoopsDropdown.setJustificationType(juce::Justification::centred);

    savedLoopsDropdown.setTextWhenNothingSelected("  NO PROJECT LOADED");
    savedLoopsDropdown.setTextWhenNoChoicesAvailable("NO PROJECTS FOUND");
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

    unsavedProgressWarning.setLookAndFeel(&customLookAndFeel);
    unsavedProgressWarning.addButton("Cancel", 0);
    unsavedProgressWarning.addButton("OK", 1);

    unsavedProgressWarning.addKeyListener(this);
    saveProjectDialog.addKeyListener(this);  //DN:these are so we can hit Enter on the alert windows

    //sets up the alert window for when you hit Save
    saveProjectDialog.setLookAndFeel(&customLookAndFeel);
    saveProjectDialog.addTextEditor("newProjectName", "");
    saveProjectDialog.addButton("Cancel", 0);
    saveProjectDialog.addButton("Save", 1);

    // Make sure you set the size of the component after
    // you add any child components.
    setSize(960, 700);
}

MainComponent::~MainComponent()
{
    deviceManager.removeChangeListener(this);
    setLookAndFeel(nullptr);
    unsavedProgressWarning.setLookAndFeel(nullptr);
    saveProjectDialog.setLookAndFeel(nullptr);
    audioSetupComp->setLookAndFeel(nullptr);
    // This shuts down the audio device and clears the audio source.
    shutdownAudio();
}

//==============================================================================
void MainComponent::prepareToPlay (int samplesPerBlockExpected, double sampleRate)
{
    mixer.prepareToPlay(samplesPerBlockExpected, sampleRate);
}

void MainComponent::getNextAudioBlock (const juce::AudioSourceChannelInfo& bufferToFill)
{
    auto* device = deviceManager.getCurrentAudioDevice();
    auto activeInputChannels = device->getActiveInputChannels();
    auto activeOutputChannels = device->getActiveOutputChannels();
    auto maxInputChannels = activeInputChannels.countNumberOfSetBits();

    //DN: force input to be 1 channel only initially
    if (!settingsHaveBeenOpened && maxInputChannels > 1)
        maxInputChannels = 1;
        
    auto maxOutputChannels = activeOutputChannels.countNumberOfSetBits();
    auto test = device->getInputChannelNames();

    auto sourceBuffer = std::make_unique<juce::AudioBuffer<float>>(maxInputChannels, bufferToFill.numSamples);

    /// DN: This code grabs the audio input, puts it in a buffer, and sends that to an AudioSource class
    //  which can be added to or removed from our main mixer
  
     //DN: if input is mono, this will write the same data twice, but that's ok
     // we need to account for the outputChannels in case it's set to mono or nothing
    //even though we're not writing to the output buffer here
    if (maxInputChannels == 0)
    {
        for (auto channel = 0; channel < maxOutputChannels; ++channel)
            bufferToFill.buffer->clear(channel, bufferToFill.startSample, bufferToFill.numSamples);

        sourceBuffer->clear();
    }
    for (auto channel = 0; channel < maxInputChannels; ++channel)
    {
        //DN: get the input and fill that channel of our source buffer
        auto* reader = bufferToFill.buffer->getReadPointer(channel,
            bufferToFill.startSample);

        auto* writer = sourceBuffer->getWritePointer(channel, bufferToFill.startSample);

        for (auto sample = 0; sample < bufferToFill.numSamples; ++sample)
            writer[sample] = reader[sample];

    }

    //send filled buffer to the AudioSource
    inputAudio.setBuffer(sourceBuffer.release());

    //DN: This gets the audio from everything that's been added to the mixer and sends it to the output
    mixer.getNextAudioBlock(bufferToFill);
}

void MainComponent::releaseResources()
{
    mixer.releaseResources();
}

//==============================================================================
void MainComponent::paint (juce::Graphics& g)
{
    g.fillAll(MAIN_BACKGROUND_COLOR);
}

void MainComponent::resized()
{
    auto mainFullOuterBorder = 13;
    auto rect = getLocalBounds().reduced(mainFullOuterBorder);
    auto extraRightMargin = rect.removeFromRight(5);
    auto leftColumnWidth = 260;
    auto titleArea = rect.removeFromTop(40);
    appTitle.setBounds(titleArea.removeFromLeft(leftColumnWidth).reduced(10));
    settingsButton.setBounds(titleArea.removeFromRight(50).reduced(2));

    int headerHeight = 120;
    auto headerArea = rect.removeFromTop(headerHeight);

    auto transportButtonArea = headerArea.removeFromLeft(leftColumnWidth-5);
    int playStopMargin = 20;
    transportButtonArea.removeFromLeft(playStopMargin);
    transportButtonArea.removeFromRight(playStopMargin);

    stopButton.setBounds(transportButtonArea.removeFromLeft(transportButtonArea.getWidth()/2).reduced(0, headerHeight*0.22f));
    playButton.setBounds(transportButtonArea.reduced(0, headerHeight*0.22f));


    savedLoopsDropdown.setBounds(headerArea.removeFromRight(350).reduced(8,headerHeight*0.33f));
    int saveClearButtonsWidth = 55;

    auto saveButtonArea = headerArea.removeFromRight(saveClearButtonsWidth).reduced(5, headerHeight * 0.35f);
    saveButton.setBounds(saveButtonArea); 
    auto initializeButtonBounds = headerArea.removeFromRight(saveClearButtonsWidth);
    initializeButton.setBounds(initializeButtonBounds.reduced(5, headerHeight * 0.35f).reduced(3));
    plusIcon.setBounds(initializeButtonBounds.reduced(17));
    auto metronomeArea = headerArea.removeFromRight(saveClearButtonsWidth).reduced(5, headerHeight * 0.33f);
    metronomeButton.setBounds(metronomeArea.removeFromTop(metronomeArea.getHeight()*0.98));
    int boxWidth = 80;
    auto cutSliverAboveTempoBeats = headerArea.removeFromTop(5);
    tempoBox.setBounds(headerArea.removeFromRight(boxWidth).reduced(10, headerHeight * 0.33f));
    beatsBox.setBounds(headerArea.removeFromRight(boxWidth).reduced(10, headerHeight * 0.33f));

    rect.expand(mainFullOuterBorder,mainFullOuterBorder);
    auto loopControllerRow = rect.removeFromTop(25);
    loopLengthButton.setBounds(loopControllerRow.removeFromRight(46));

    rect.reduce(mainFullOuterBorder,mainFullOuterBorder);
    for (auto& track : tracksArray)
    {
        auto trackArea = rect.removeFromTop(120);
        auto trackControlsL = trackArea.removeFromLeft(200);
        track->recordButton.setBounds(trackControlsL.removeFromLeft(80).reduced(8));
        track->panSlider.setBounds(trackControlsL.removeFromLeft(60));
        track->gainSlider.setBounds(trackControlsL.removeFromLeft(60).reduced(15,0));
        auto trackControlsR = trackArea.removeFromLeft(leftColumnWidth-200);
        trackControlsR.reduce(0, 42);
        track->reverseButton.setBounds(trackControlsR);
        track->setBounds(trackArea);
    }
}


// AF: ========================= Audio Playing Declarations ================================

void MainComponent::changeListenerCallback(juce::ChangeBroadcaster* source)
{
    for (auto& track : tracksArray)
    {
        if (source == track)
        {
            //DN:  we need this to handle things if recording is cut off automatically
            //DN: this will also be hit whenever any of the track controls are clicked on
            //DN: and when you initially hit the play button, so be careful what is added here
            if (!trackCurrentlyRecording())
            {
                for (auto& i : tracksArray)
                {
                    i->setDisplayFullThumbnail(true);
                    i->recordButton.setEnabled(true);
                    i->setShouldLightUp(false);
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
}

void MainComponent::stopButtonClicked()
{
    changeState(Stopping);

    // AF: Stop tracks if stop button is clicked
    for (auto& track : tracksArray)
    {
        track->setWaitingToRecord(false);
        if (track->isRecording())
        {
            track->stopRecording();
        }
    }
}

// AF: ========================= Save/Load Declarations ================================


void MainComponent::saveButtonClicked()
{
    juce::String newFolderName;
    bool isNewProject = savedLoopsDropdown.getSelectedId() == 0;

    //DN: if this project is new, we need to name it/create a folder for it
    if (isNewProject)
    {
        auto result = saveProjectDialog.runModalLoop();
        if (result == 1)
        {
            unsavedChanges = false;
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
        else
        {
            saveProjectDialog.setVisible(false);
            return;
        }

        saveProjectDialog.setVisible(false);
        //DN: the dialog box just goes behind everything until it gets called to the front again
        //    so we need to clear the text box for next time
        auto textEditor = saveProjectDialog.getTextEditor("newProjectName");
        textEditor->setText("");
    }
    else
    {
        unsavedChanges = false;
        newFolderName = savedLoopsDropdown.getText();
        savedLoopDirTree.saveWAVsTo(newFolderName);  //DN: save loop to project folder selected in dropdown
    }

    //DN: now we refresh the dropdown list with the current folders
    //and find the folder we just saved To, then make it the current selection
    juce::StringArray folderNames = savedLoopDirTree.getLoopFolderNamesArray();
    savedLoopsDropdown.clear(juce::dontSendNotification);
    savedLoopsDropdown.addItemList(folderNames,1); //DN: set first item index offset to 1, 0 will be when no project is selected
    for (int i = 0; i < folderNames.size(); ++i)
    {
        if (folderNames[i] == newFolderName)
        {
            //account for dropdown index offset, don't trigger savedLoopSelected
            savedLoopsDropdown.setSelectedId(i + 1, juce::dontSendNotification); 
            currentProjectListID = i+1;
        }
            
    }

    //DN: save the track states to an xml file in the project folder
    // create an outer node
    juce::XmlElement projectState("projectState");
    projectState.setAttribute("tempo", tempoBox.getText().getIntValue());
    projectState.setAttribute("beats", beatsBox.getText().getIntValue());

    for (int i = 0; i < NUM_TRACKS; ++i)
    {
        projectState.addChildElement(tracksArray[i]->getTrackState(i+1));
    }

    //write it to a file in this project's folder
    juce::File destFile = savedLoopDirTree.getProjectFolder(newFolderName).getChildFile(PROJECT_STATE_XML_FILENAME);
    
    projectState.writeTo(destFile);


    //Need feedback if you hit save on an existing project
    if (!isNewProject)
    {
        juce::AlertWindow savedNotification("Project Saved!", "", juce::AlertWindow::NoIcon);
        savedNotification.setLookAndFeel(&customLookAndFeel);
        savedNotification.addButton("OK", 1);
        savedNotification.runModalLoop();
    }

}

void MainComponent::initializeButtonClicked()
{
    //creates a warning here that will abort this function if they hit Cancel
    if (unsavedChanges)
    {
        auto result = unsavedProgressWarning.runModalLoop();
        unsavedProgressWarning.setVisible(false);
        if (result == 0)
        {
            return;
        }
    }

    savedLoopsDropdown.setSelectedId(0,juce::dontSendNotification);
    currentProjectListID = 0;
    initializeTempWAVs();
    redrawAndBufferAudio();
    tempoBox.setReadOnly(false);
    tempoBox.setEnabled(true);
    tempoBox.setColour(juce::TextEditor::textColourId, MAIN_DRAW_COLOR);
    auto text = tempoBox.getText();
    tempoBox.clear();
    tempoBox.setText(text);
    tempoBoxLabel.setColour(juce::Label::textColourId, MAIN_DRAW_COLOR);
    for (auto& track : tracksArray)
    {
        track->initializeTrackState();
    }
    unsavedChanges = false;
}

void MainComponent::settingsButtonClicked()
{
    settingsHaveBeenOpened = true;
    for (auto& track : tracksArray)
    {

        track->setSettingsHaveBeenOpened(true);
    }
    //DN: set up settings window
    settingsWindow.content.setNonOwned(audioSetupComp.get());

    settingsWindow.content->setSize(600, 400);
    settingsWindow.content->setColour(juce::ComboBox::backgroundColourId, MAIN_BACKGROUND_COLOR);
    settingsWindow.content->setColour(juce::ComboBox::outlineColourId, MAIN_DRAW_COLOR);
    settingsWindow.content->setColour(juce::ComboBox::textColourId, MAIN_DRAW_COLOR);
    settingsWindow.dialogTitle = "Settings";
    settingsWindow.dialogBackgroundColour = MAIN_BACKGROUND_COLOR;
    settingsWindow.escapeKeyTriggersCloseButton = true;
    settingsWindow.useNativeTitleBar = true;
    settingsWindow.resizable = false;

    settingsWindow.runModal();
}

void MainComponent::savedLoopSelected()
{
    //creates a warning here that will reset the dropdown and abort this function if they hit Cancel
    if (savedLoopsDropdown.getSelectedId() != 0 && unsavedChanges)
    {
        auto result = unsavedProgressWarning.runModalLoop();
        unsavedProgressWarning.setVisible(false);
        if (result == 0)
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
        //DN: restore global project settings
        tempoBox.setText(juce::String(projectState->getIntAttribute("tempo")));
        beatsBox.setText(juce::String(projectState->getIntAttribute("beats")));


        //iterate through xml and restore the state of each track
        for (int i = 0; i < NUM_TRACKS; ++i)
        {
            forEachXmlChildElement(*projectState, trackState)
            {
                juce::String trackName = TRACK_FILENAME + juce::String(i + 1);
                if (trackState->hasTagName(trackName))
                    tracksArray[i]->restoreTrackState(trackState);
            }

            tracksArray[i]->setMasterLoop(tempoBox.getText().getIntValue(), beatsBox.getText().getIntValue());
        }
    }

    currentProjectListID = savedLoopsDropdown.getSelectedId();
    unsavedChanges = false;

    tempoBox.setEnabled(false);
    tempoBox.setColour(juce::TextEditor::textColourId, SECONDARY_DRAW_COLOR);
    auto text = tempoBox.getText();
    tempoBox.clear();
    tempoBox.setText(text);
    tempoBoxLabel.setEnabled(false);
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

// =============================== MISC ============================================

void MainComponent::metronomeButtonClicked()
{
    if (metronome.getState() == Metronome::Stopped)
    {
        metronome.start();
        metronomeSVG->replaceColour(MAIN_DRAW_COLOR, METRONOME_ON_COLOR);
        metronomeButton.setImages(metronomeSVG.get());
    }
    else if (metronome.getState() == Metronome::Playing)
    {
        metronome.stop();
        metronomeSVG->replaceColour(METRONOME_ON_COLOR, MAIN_DRAW_COLOR);
        metronomeButton.setImages(metronomeSVG.get());
    }
}

bool MainComponent::keyPressed(const juce::KeyPress& key, juce::Component* originatingComponent)
{
    if (originatingComponent == &unsavedProgressWarning || originatingComponent == &saveProjectDialog)
    {
        if (key == juce::KeyPress::returnKey)
            originatingComponent->exitModalState(1);  //DN: if someone hits enter on an Alertwindow, it's the same as clicking OK
        else if (key == juce::KeyPress::escapeKey)
            originatingComponent->exitModalState(0);
    }
    else if (key == juce::KeyPress::spaceKey)
    {
        if (trackCurrentlyPlaying())
            stopButtonClicked();
        else
            playButtonClicked();
    }

    return true;
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
        if (track->isRecording() || track->isWaitingToRecord())
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
            settingsButton.setEnabled(true);
            loopLengthButton.setEnabled(true);
            stopButton.setEnabled(false);
            playButton.setEnabled(true);
            savedLoopsDropdown.setEnabled(true);
            savedLoopsDropdown.setColour(juce::ComboBox::outlineColourId, MAIN_DRAW_COLOR);
            saveButton.setEnabled(true);
            initializeButton.setEnabled(true);
            initializeButton.setOutline(MAIN_DRAW_COLOR, NEW_FILE_LINE_THICKNESS);
            plusIcon.setEnabled(true);
            for (auto& track : tracksArray)
            {
                track->setPosition(0);
            }
            break;

        case Starting: 
            metronome.reset();
            settingsButton.setEnabled(false);
            loopLengthButton.setEnabled(false);
            playButton.setEnabled(false);
            playButton.setOutline(juce::Colours::limegreen, PLAY_STOP_LINE_THICKNESS);
            savedLoopsDropdown.setEnabled(false);
            savedLoopsDropdown.setColour(juce::ComboBox::outlineColourId, SECONDARY_DRAW_COLOR);
            saveButton.setEnabled(false);
            initializeButton.setEnabled(false);
            initializeButton.setOutline(SECONDARY_DRAW_COLOR, NEW_FILE_LINE_THICKNESS);
            plusIcon.setEnabled(false);
            for (auto& track : tracksArray)
            {
                track->start();
            }
            break;

        case Playing:                           
            stopButton.setEnabled(true);
            break;

        case Stopping:
            playButton.setOutline(MAIN_DRAW_COLOR, PLAY_STOP_LINE_THICKNESS);
            metronome.stop();
            metronomeSVG->replaceColour(METRONOME_ON_COLOR, MAIN_DRAW_COLOR);
            metronomeButton.setImages(metronomeSVG.get());
            for (auto& track : tracksArray)
            {
                track->stop();
            }
            break;

        }
    }
}

// AF: Text Box Listeners
void MainComponent::textEditorReturnKeyPressed(juce::TextEditor &textEditor)
{
    if (&textEditor == &tempoBox)
    {
        metronome.setBpm(textEditor.getText().getIntValue());
        for (auto& track : tracksArray)
            track->setMasterLoop(tempoBox.getText().getIntValue(), beatsBox.getText().getIntValue());
    }

    if (&textEditor == &beatsBox)
    {
        for (auto& track : tracksArray)
            track->setMasterLoop(tempoBox.getText().getIntValue(), beatsBox.getText().getIntValue());
    }

    juce::Component::unfocusAllComponents();
}
void MainComponent::textEditorFocusLost(juce::TextEditor &textEditor)
{
    if (&textEditor == &tempoBox)
    {
        metronome.setBpm(textEditor.getText().getIntValue());
        for (auto& track : tracksArray)
            track->setMasterLoop(tempoBox.getText().getIntValue(), beatsBox.getText().getIntValue());

        //DN: trying to un-highlight when you click away
        int oldValue = tempoBox.getText().getIntValue();
        tempoBox.clear();
        tempoBox.setText(juce::String(oldValue));
        tempoBox.setHighlightedRegion(juce::Range<int>().withStartAndLength(0,0));
    }

    if (&textEditor == &beatsBox)
    {
        for (auto& track : tracksArray)
            track->setMasterLoop(tempoBox.getText().getIntValue(), beatsBox.getText().getIntValue());
        
        //DN: only way to un-highlight when you click away
        int oldValue = beatsBox.getText().getIntValue();
        beatsBox.clear();
        beatsBox.setText(juce::String(oldValue));
        beatsBox.setHighlightedRegion(juce::Range<int>().withStartAndLength(0, 0));
    }

    juce::Component::unfocusAllComponents();
}

void MainComponent::textEditorTextChanged(juce::TextEditor& textEditor)
{

    if (&textEditor == &beatsBox && beatsBox.getText().getIntValue() > 1)
    {
        int newBeats = beatsBox.getText().getIntValue();
        DBG("textChanged " + juce::String(newBeats));
        int newTempo = tempoBox.getText().getIntValue();
        for (auto& track : tracksArray)
        {
            DBG("changing track looplength");
            track->setMasterLoop(newTempo, newBeats);
            track->repaint();
        }
    }
}
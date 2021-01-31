#include "MainComponent.h"

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
    addAndMakeVisible(recordButton);
    recordButton.setColour(juce::TextButton::buttonColourId, juce::Colour(0xffff5c5c));
    recordButton.setColour(juce::TextButton::textColourOnId, juce::Colours::black);

    // AF: Adds play button and paints it
    addAndMakeVisible(&playButton);
    playButton.onClick = [this] { playButtonClicked(); };
    playButton.setColour(juce::TextButton::buttonColourId, juce::Colours::green);
    playButton.setEnabled(false);

    // AF: Adds stop button and paints it
    addAndMakeVisible(&stopButton);
    stopButton.onClick = [this] { stopButtonClicked(); };
    stopButton.setColour(juce::TextButton::buttonColourId, juce::Colours::red);
    stopButton.setEnabled(false);

    recordButton.onClick = [this]
    {
        if (recorder.isRecording())
            stopRecording();
        else
            startRecording();
    };

    // AF: This registers the basic formats WAV and AIFF to be read.
    // AF: Without this, the code throws an exception because it doesn't know how to read the file.
    formatManager.registerBasicFormats();
    transportSource.addChangeListener(this);   

    addAndMakeVisible(recordingThumbnail);



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

    deviceManager.addAudioCallback(&recorder);

    deviceManager.addChangeListener(this);  // from audioDeviceManager tutorial, listener is currently unused but will need

    // Make sure you set the size of the component after
    // you add any child components.
    setSize(800, 600);
}

MainComponent::~MainComponent()
{
    deviceManager.removeChangeListener(this);
    deviceManager.removeAudioCallback(&recorder);
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

    // For more details, see the help for AudioProcessor::prepareToPlay()
    
    // AF: This is new as a part of adding play button functionality
    transportSource.prepareToPlay(samplesPerBlockExpected, sampleRate);
}

void MainComponent::getNextAudioBlock (const juce::AudioSourceChannelInfo& bufferToFill)
{
    // Your audio-processing code goes here!
    // For more details, see the help for AudioProcessor::getNextAudioBlock()

    // Right now we are not producing any data, in which case we need to clear the buffer
    // (to prevent the output of random noise)
    
    //bufferToFill.clearActiveBufferRegion();  //dan commented out since processing has been added below from audioDeviceManagerTutorial:

    auto* device = deviceManager.getCurrentAudioDevice();

    auto activeInputChannels = device->getActiveInputChannels();
    auto activeOutputChannels = device->getActiveOutputChannels();

    auto maxInputChannels = activeInputChannels.getHighestBit() + 1;
    auto maxOutputChannels = activeOutputChannels.getHighestBit() + 1;

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
                auto* inBuffer = bufferToFill.buffer->getReadPointer(actualInputChannel,
                    bufferToFill.startSample);
                auto* outBuffer = bufferToFill.buffer->getWritePointer(channel, bufferToFill.startSample);

                for (auto sample = 0; sample < bufferToFill.numSamples; ++sample)
                    outBuffer[sample] = inBuffer[sample];// * random.nextFloat() * 0.25f;
            }
        }
    }

    // AF: ================== Play button functionality ==================
    if (readerSource.get() == nullptr)
    {
        bufferToFill.clearActiveBufferRegion();
        return;
    }

    transportSource.getNextAudioBlock(bufferToFill);
    // AF: ================== Play button functionality ==================
}

void MainComponent::releaseResources()
{
    // This will be called when the audio device stops, or when it is being
    // restarted due to a setting change.

    // For more details, see the help for AudioProcessor::releaseResources()

    // AF: ================== Play button functionality ==================
    transportSource.releaseResources();
    // AF: ================== Play button functionality ==================
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
    auto rect = getLocalBounds();

    audioSetupComp.setBounds(rect.removeFromLeft(proportionOfWidth(0.6f)));
    rect.reduce(10, 10);

    rect.removeFromTop(20);

    recordingThumbnail.setBounds(rect.removeFromTop(80).reduced(8));

    // AF: This is what actually makes the buttons visible
    recordButton.setBounds(rect.removeFromTop(36).removeFromLeft(140).reduced(8));
    playButton.setBounds(rect.removeFromTop(36).removeFromLeft(140).reduced(8));
    stopButton.setBounds(rect.removeFromTop(36).removeFromLeft(140).reduced(8));


}

// AF: This function is essentially 'openButtonClicked()' from the 'PlayingSoundFilesTutorial', but modified
// to get the lastRecording file instead of asking the user for a file
void MainComponent::recordingSaved()
{
    auto file = lastRecording;
    auto* reader = formatManager.createReaderFor(file);
    if (reader != nullptr)
    {
        std::unique_ptr<juce::AudioFormatReaderSource> newSource(new juce::AudioFormatReaderSource(reader, true));
        transportSource.setSource(newSource.get(), 0, nullptr, reader->sampleRate);                                 
        playButton.setEnabled(true);                                                                                
        readerSource.reset(newSource.release());
    }
}

void MainComponent::startRecording()
{
    if (!juce::RuntimePermissions::isGranted(juce::RuntimePermissions::writeExternalStorage))
    {
        SafePointer<MainComponent> safeThis(this);

        juce::RuntimePermissions::request(juce::RuntimePermissions::writeExternalStorage,
            [safeThis](bool granted) mutable
            {
                if (granted)
                    safeThis->startRecording();
            });
        return;
    }

#if (JUCE_ANDROID || JUCE_IOS)
    auto parentDir = juce::File::getSpecialLocation(juce::File::tempDirectory);
#else
    // AF: Here it seems the user's "Documents" path is stored in parentDir
    auto parentDir = juce::File::getSpecialLocation(juce::File::userDocumentsDirectory);
#endif

    // AF: Here it seems like the program initializes the file for recording
    lastRecording = parentDir.getNonexistentChildFile("JUCE Demo Audio Recording", ".wav");

    recorder.startRecording(lastRecording);

    recordButton.setButtonText("Stop");
    recordingThumbnail.setDisplayFullThumbnail(false);
}

void MainComponent::stopRecording()
{
    recorder.stop();
    // AF: This activates the play button
    recordingSaved();

    // AF: Ignoring this for now as it seems like something not relevant to the app
#if JUCE_CONTENT_SHARING
    SafePointer<MainComponent> safeThis(this);
    juce::File fileToShare = lastRecording;

    juce::ContentSharer::getInstance()->shareFiles(juce::Array<juce::URL>({ juce::URL(fileToShare) }),
        [safeThis, fileToShare](bool success, const juce::String& error)
        {
            if (fileToShare.existsAsFile())
                fileToShare.deleteFile();

            if (!success && error.isNotEmpty())
            {
                juce::NativeMessageBox::showMessageBoxAsync(juce::AlertWindow::WarningIcon,
                    "Sharing Error",
                    error);
            }
        });
#endif

    // lastRecording = juce::File();
    recordButton.setButtonText("Record");
    recordingThumbnail.setDisplayFullThumbnail(true);
}

// AF: ========================= New Audio Playing Declarations ================================

void MainComponent::changeListenerCallback(juce::ChangeBroadcaster* source)
{
    if (source == &transportSource)
    {
        if (transportSource.isPlaying())
            changeState(Playing);
        else
            changeState(Stopped);
    }
}

void MainComponent::playButtonClicked()
{
    changeState(Starting);
}

void MainComponent::stopButtonClicked()
{
    changeState(Stopping);
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
            transportSource.setPosition(0.0);
            break;

        case Starting:                          
            playButton.setEnabled(false);
            transportSource.start();
            break;

        case Playing:                           
            stopButton.setEnabled(true);
            break;

        case Stopping:                          
            transportSource.stop();
            break;
        }
    }
}
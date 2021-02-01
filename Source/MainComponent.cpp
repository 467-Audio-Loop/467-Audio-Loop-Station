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
    addAndMakeVisible(track1RecordButton);
    track1RecordButton.setColour(juce::TextButton::buttonColourId, juce::Colour(0xffff5c5c));
    track1RecordButton.setColour(juce::TextButton::textColourOnId, juce::Colours::black);

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

    track1RecordButton.onClick = [this]
    {
        if (track1Recorder.isRecording())
            stopRecording();
        else
            startRecording();
    };

    // AF: This registers the basic formats WAV and AIFF to be read.
    // AF: Without this, the code throws an exception because it doesn't know how to read the file.
    formatManager.registerBasicFormats();
    track1TransportSource.addChangeListener(this);   

    

    addAndMakeVisible(track1RecordingThumbnail);



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

    deviceManager.addAudioCallback(&track1Recorder);

    deviceManager.addChangeListener(this);  // DN: from audioDeviceManager tutorial, listener is currently unused but will need

    // Make sure you set the size of the component after
    // you add any child components.
    setSize(1000, 800);
}

MainComponent::~MainComponent()
{
    deviceManager.removeChangeListener(this);
    deviceManager.removeAudioCallback(&track1Recorder);
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
    track1TransportSource.prepareToPlay(samplesPerBlockExpected, sampleRate);
}

void MainComponent::getNextAudioBlock (const juce::AudioSourceChannelInfo& bufferToFill)
{

    // DN: ================== Audio passthrough code, input goes to output ==================
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

    // DN: I commented this out, it was erasing the above audio passthrough
    //      and I think what it's trying to catch is handled above
    /*if (track1ReaderSource.get() == nullptr)
    {
        bufferToFill.clearActiveBufferRegion();  
        return;
    }*/

    //DN: Instead let's check the state and passthrough when not playing
    if (!track1TransportSource.isPlaying())
    {
        return;
    }

    track1TransportSource.getNextAudioBlock(bufferToFill);
    // AF: ================== Play button functionality ==================
}

void MainComponent::releaseResources()
{
    // This will be called when the audio device stops, or when it is being
    // restarted due to a setting change.

    // For more details, see the help for AudioProcessor::releaseResources()

    // AF: ================== Play button functionality ==================
    track1TransportSource.releaseResources();
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
    auto globalControlsArea = rect.removeFromTop(400);

    audioSetupComp.setBounds(globalControlsArea.removeFromLeft(proportionOfWidth(0.5f)));
    auto transportControlsArea = globalControlsArea.removeFromTop(40);
    stopButton.setBounds(transportControlsArea.removeFromLeft(140).reduced(8));
    playButton.setBounds(transportControlsArea.removeFromLeft(140).reduced(8));
    

    //audioSetupComp.setBounds(rect.removeFromLeft(proportionOfWidth(0.6f)));
    //rect.reduce(10, 10);

    
    auto track1Area = rect.removeFromTop(80);
    track1RecordButton.setBounds(track1Area.removeFromLeft(140).reduced(10));
    track1RecordingThumbnail.setBounds(track1Area.reduced(8));



}

// AF: This function is essentially 'openButtonClicked()' from the 'PlayingSoundFilesTutorial', but modified
// to get the lastRecording file instead of asking the user for a file
void MainComponent::recordingSaved()
{
    auto file = track1LastRecording;
    auto* reader = formatManager.createReaderFor(file);
    if (reader != nullptr)
    {
        std::unique_ptr<juce::AudioFormatReaderSource> newSource(new juce::AudioFormatReaderSource(reader, true));
        newSource->setLooping(true);  //DN: added this to make the reader loop, the transportSource then inherits this behavior
        track1TransportSource.setSource(newSource.get(), 0, nullptr, reader->sampleRate);                                 
        playButton.setEnabled(true);                                                                                
        track1ReaderSource.reset(newSource.release());
   
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

    
    //DN: added "if" so that after the initial file is made, subsequent records will overwrite it
    if (!track1LastRecording.exists())
    {
        // AF: Here the program initializes the file for recording
        track1LastRecording = parentDir.getNonexistentChildFile("LoopStation-Track1", ".wav");
    }

    track1Recorder.startRecording(track1LastRecording);
    track1RecordButton.setButtonText("Stop");
    track1RecordingThumbnail.setDisplayFullThumbnail(false);
}

void MainComponent::stopRecording()
{
    track1Recorder.stop();

    recordingSaved();

    track1RecordButton.setButtonText("Record");
    track1RecordingThumbnail.setDisplayFullThumbnail(true);
}

// AF: ========================= New Audio Playing Declarations ================================

void MainComponent::changeListenerCallback(juce::ChangeBroadcaster* source)
{
    if (source == &track1TransportSource)
    {
        if (track1TransportSource.isPlaying())
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
            track1TransportSource.setPosition(0.0);
            break;

        case Starting:                          
            playButton.setEnabled(false);
            track1TransportSource.start();
            break;

        case Playing:                           
            stopButton.setEnabled(true);
            break;

        case Stopping:                          
            track1TransportSource.stop();
            break;
        }
    }
}
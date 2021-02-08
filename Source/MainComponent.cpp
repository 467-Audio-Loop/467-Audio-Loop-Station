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
    playButton.setEnabled(false);

    // AF: Adds stop button and paints it
    addAndMakeVisible(&stopButton);
    stopButton.onClick = [this] { stopButtonClicked(); };
    stopButton.setColour(juce::TextButton::buttonColourId, juce::Colours::red);
    stopButton.setEnabled(false);

    track1RecordButton.onClick = [this]
    {
        if (track1.isRecording())
        {
            track1.stopRecording();
            // AF: Only enable play button if no tracks are currently playing.
            if (state == Stopped)
            {
                playButton.setEnabled(true);
            }
            else // AF: If playback is happening, start playing this track
            {
                track1.start();
            }
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

            // AF: Stop track from playing if this current track is actively playing,
            // before starting to record again over it
            if (track1.isPlaying())
                track1.stop();


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
        }
    };

    track2RecordButton.onClick = [this]
    {
        if (track2.isRecording())
        {
            track2.stopRecording();
            // AF: Only enable play button if no tracks are currently playing.
            if (state == Stopped)
            {
                playButton.setEnabled(true);
            }
            else // AF: If playback is happening, go ahead and start this track
            {
                track2.start();
            }
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

            // AF: Stop track from playing if this current track is actively playing,
            // before starting to record again over it
            if (track2.isPlaying())
                track2.stop();

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
        }
    };

    track3RecordButton.onClick = [this]
    {
        if (track3.isRecording())
        {
            track3.stopRecording();
            // AF: Only enable play button if no tracks are currently playing.
            if (state == Stopped)
            {
                playButton.setEnabled(true);
            }
            else // AF: If playback is happening, go ahead and start this track
            {
                track3.start();
            }
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

            // AF: Stop track from playing if this current track is actively playing,
            // before starting to record again over it
            if (track3.isPlaying())
                track3.stop();

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
        }
    };

    track4RecordButton.onClick = [this]
    {
        if (track4.isRecording())
        {
            track4.stopRecording();
            // AF: Only enable play button if no tracks are currently playing.
            if (state == Stopped)
            {
                playButton.setEnabled(true);
            }
            else // AF: If playback is happening, go ahead and start this track
            {
                track4.start();
            }

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

            // AF: Stop track from playing if this current track is actively playing,
            // before starting to record again over it
            if (track4.isPlaying())
                track4.stop();

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
        }
    };

    track1.addChangeListener(this);
    track2.addChangeListener(this);
    track3.addChangeListener(this);
    track4.addChangeListener(this);

    addAndMakeVisible(track1);
    addAndMakeVisible(track2);
    addAndMakeVisible(track3);
    addAndMakeVisible(track4);

    mixer.addInputSource(&track1, false);
    mixer.addInputSource(&track2, false);
    mixer.addInputSource(&track3, false);
    mixer.addInputSource(&track4, false);



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
    if (!trackCurrentlyPlaying() && !trackCurrentlyRecording())
    {
        return;
    }

    //DN: We've added the tracks to the mixer already, so this will trigger all of them
    mixer.getNextAudioBlock(bufferToFill);



    // AF: ================== Play button functionality ==================
}

void MainComponent::releaseResources()
{
    // This will be called when the audio device stops, or when it is being
    // restarted due to a setting change.

    // For more details, see the help for AudioProcessor::releaseResources()

    // AF: ================== Play button functionality ==================
    track1.releaseResources();
    track2.releaseResources();
    track3.releaseResources();
    track4.releaseResources();
    mixer.releaseResources();
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
    if (source == &track1)
    {
        // AF: This if statement needs to happen in order to prevent buttons from changing states
        // when user tries to record while audio is playing
        if (!track1.isRecording())
        {
            if (track1.isPlaying())
                changeState(Playing);
            else
                changeState(Stopped);
        }
    }

    if (source == &track2)
    {
        if (!track2.isRecording())
        {
            if (track2.isPlaying())
                changeState(Playing);
            else
                changeState(Stopped);
        }
    }

    if (source == &track3)
    {
        if (!track3.isRecording())
        {
            if (track3.isPlaying())
                changeState(Playing);
            else
                changeState(Stopped);
        }
    }

    if (source == &track4)
    {
        if (!track4.isRecording())
        {
            if (track4.isPlaying())
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
            track1.setPosition(0);
            track2.setPosition(0);
            track3.setPosition(0);
            track4.setPosition(0);
            break;

        case Starting:                          
            playButton.setEnabled(false);
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
/*
  ==============================================================================

    AudioTrack.h

    A class to represent an Audio Track.  Contains all track-specific controls and
    methods.  Inherits from AudioIODeviceCallback to allow passing input data to
    the Audio Recorder class, parallel to our main audio processing chain.

  ==============================================================================
*/

#pragma once

#include "AudioRecorder.h"
#include "LoopSource.h"
#include "SaveLoad.h"
#include "customUI.h"


class AudioTrack : public juce::AudioAppComponent,
    private juce::ChangeListener, public juce::ChangeBroadcaster, public juce::AudioIODeviceCallback,
    public juce::Slider::Listener, public juce::Button::Listener, private juce::Timer, public juce::MouseListener
{
public:
    AudioTrack()
    {
        formatManager.registerBasicFormats();
        thumbnail.addChangeListener(this);
        loopSource.addChangeListener(this);
        deviceManager.addAudioCallback(&recorder);   

        // AF: Initialize track sliders
        panSlider.setRange(-1.0, 1.0);
        panSlider.setValue(0.0);
        panSlider.addListener(this);
        panSlider.setDoubleClickReturnValue(true, 0.0, juce::ModifierKeys::altModifier);
        panSlider.setTextBoxStyle(juce::Slider::TextEntryBoxPosition::NoTextBox, true, 0, 0);

        slipController.addListener(this);
        slipController.setRange(-loopSource.getMasterLoopLength(), loopSource.getMasterLoopLength());
        slipController.setValue(0);
        slipController.setDoubleClickReturnValue(true, 0.0, juce::ModifierKeys::altModifier);

        gainSlider.setRange(0.0, 1.0);
        gainSlider.setValue(1.0);
        gainSlider.addListener(this);
        gainSlider.setDoubleClickReturnValue(true, 1.0, juce::ModifierKeys::altModifier);
        gainSlider.setSliderStyle(juce::Slider::SliderStyle::LinearVertical);
        gainSlider.setTextBoxStyle(juce::Slider::TextEntryBoxPosition::NoTextBox, true, 0, 0);

        reverseButton.addListener(this);

        startTimer(10); //fires every 10ms
    }


    ~AudioTrack() override
    {
        thumbnail.removeChangeListener(this);
        deviceManager.removeAudioCallback(&recorder);
    }

    //DN:: Gives this class access to the buffer
    void audioDeviceIOCallback(const float** inputChannelData,
        int numInputChannels,
        float** outputChannelData,
        int numOutputChannels,
        int numSamples) override
    {
        if (!settingsHaveBeenOpened && numInputChannels > 1)
            numInputChannels = 1;
        recorder.audioDeviceIOCallback(inputChannelData, numInputChannels,outputChannelData, numOutputChannels,numSamples);
    }

    void audioDeviceAboutToStart(juce::AudioIODevice* device) override
    {
        recorder.audioDeviceAboutToStart(device);
    }

    /** Called to indicate that the device has stopped. */
    void audioDeviceStopped() override 
    {
        recorder.audioDeviceStopped();
    } 

    void setDisplayFullThumbnail(bool displayFull)
    {
        displayFullThumb = displayFull;
        repaint();
    }
    
    void paint(juce::Graphics& g) override
    {
        auto thumbnailBorder = 8;
        juce::Colour trackColor;
        g.fillAll(MAIN_BACKGROUND_COLOR);
        if (shouldLightUp)
            trackColor = juce::Colours::red;
        else
            trackColor = MAIN_DRAW_COLOR;

        g.setColour(trackColor);
        const juce::Rectangle<float> area(getLocalBounds().reduced(thumbnailBorder).toFloat());
        g.drawRoundedRectangle(area, ROUNDED_CORNER_SIZE, THIN_LINE);

        if (thumbnail.getTotalLength() > 0.0)
        {
            //DN:  paint the thumbnail audio horizontally relative to master loop and the slip offset
            auto startTime = -(double)slipController.getValue() / sampleRate;
            auto endTime = ((double)loopSource.getMasterLoopLength() / (double)sampleRate) + (double)startTime;

            auto thumbArea = getLocalBounds().reduced(thumbnailBorder);
            
            thumbnail.drawChannels(g, thumbArea, startTime, endTime, 1.0f); // 1.0f is zoom

            //DN: paint vertical line to indicate playhead position
            g.setColour(VERTICAL_LINE_COLOR);
            auto audioPosition = (float)loopSource.getPosition();
            auto drawPosition = (audioPosition / loopSource.getMasterLoopLength()) * (float)thumbArea.getWidth() + (float)thumbArea.getX();            
            g.drawLine(drawPosition, (float)thumbArea.getY()+8, drawPosition, (float)thumbArea.getBottom()-8, 2.0f);      


            //DN: horizontal line that always goes all the way across even if our audio is shorter
            g.setColour(trackColor);
            int midpoint = getLocalBounds().getHeight() / 2.0f;
            int width = getLocalBounds().getWidth();
            g.drawLine(8, midpoint, width-8, midpoint, 2);
        }
    }

    void prepareToPlay(int samplesPerBlockExpected, double newSampleRate) override 
    {
        samplesPerBlock = samplesPerBlockExpected;
        sampleRate = newSampleRate;
        loopSource.prepareToPlay(samplesPerBlockExpected, newSampleRate);
    }

    void getNextAudioBlock(const juce::AudioSourceChannelInfo& bufferToFill) override 
    {
        loopSource.getNextAudioBlock(bufferToFill);

        // AF: If only 1 output (mono), panning shouldn't work
        if (recorder.getOutputChannels() > 1)
        {

            ////DN: overall gain , applies to both channels
            int numOutputChannels = bufferToFill.buffer->getNumChannels();
            for (int channel = 0; channel < numOutputChannels; ++channel)
            {
                auto gainWriter = bufferToFill.buffer->getWritePointer(channel, bufferToFill.startSample);
                for (auto sample = 0; sample < bufferToFill.numSamples; ++sample)
                {
                    gainWriter[sample] = gainWriter[sample] * gainSliderValue;
                }
            }

            // AF:  Panning volume control //  
            // AF: This value determines the channel
            // 0 == L // 1 == R
            int channel = 0;
            double gain = 1 - fabs(panSliderValue);

            // AF: This if statement chooses which channel is going to be muted/lowered
            if (panSliderValue < 0)
                channel = 1;
            else if (panSliderValue > 0)
                channel = 0;
            else
                gain = 1.0;

            auto panWriter = bufferToFill.buffer->getWritePointer(channel, bufferToFill.startSample);

            for (auto sample = 0; sample < bufferToFill.numSamples; ++sample)
            {
                panWriter[sample] = panWriter[sample] * gain;
            }
        }


        //DN: this is where we set the bool that will auto-stop recording at the end of the loop
        if (isRecording() && (loopSource.getPosition() + samplesPerBlock >= loopSource.getMasterLoopLength()))
        {
            aboutToOverflow = true;
        }
    }

    void releaseResources() override 
    {
        loopSource.releaseResources();
    }

    void startRecording()
    {
        if (!juce::RuntimePermissions::isGranted(juce::RuntimePermissions::writeExternalStorage))
        {
            SafePointer<AudioTrack> safeThis(this);

            juce::RuntimePermissions::request(juce::RuntimePermissions::writeExternalStorage,
                [safeThis](bool granted) mutable
                {
                    if (granted)
                        safeThis->startRecording();
                });
            return;
        }

        slipController.setValue(0);

        loopSource.setBeginningOfFile(false);

        //DN:  tell loopSource to play silence/not access loopBuffer while we switch it out
        loopSource.startRecording(); 

        recorder.startRecording(lastRecording);

        setDisplayFullThumbnail(false);
    }

    void stopRecording()
    {
        recorder.stop();
        waitingToRecord = false;
        loopSource.setBeginningOfFile(false);
       
        auto file = lastRecording;
        auto reader = formatManager.createReaderFor(file);
        if (reader != nullptr)
        {
            //DN: set up a memory buffer to hold the audio for this loop file
            auto loopBuffer = std::make_unique<juce::AudioBuffer<float>>(reader->numChannels, reader->lengthInSamples);
            //DN: read the audio file into the loopBuffer
            reader->read(loopBuffer.get(), 0, reader->lengthInSamples, 0, true, true);
            //DN: need to delete here because the "createReaderFor" method says to - maybe switch to a unique ptr later for "good practice" reasons!
            delete reader; 
            // DN: send the loopBuffer object to the loopSource which will handle playback, transfer ownership of unique ptr
            loopSource.setBuffer(loopBuffer.release());
            loopSource.stopRecording();
        }
    }

    // --
    bool isRecording()
    {
        return recorder.isRecording();
    }

    bool isPlaying()
    {
        return loopSource.isPlaying();
    }

    void setShouldLightUp(bool shouldThisLightUp)
    {
        shouldLightUp = shouldThisLightUp;
    }

    // DN:  set position in samples now, not seconds anymore (made it an int, not a double)
    void setPosition(juce::int64 newPosition)
    {
        loopSource.setNextReadPosition(newPosition);
    }

    // Playback mode
    void start()
    {
        loopSource.start(0);
    }

    void stop()
    {
        loopSource.stop();
    }

    //Call this for all tracks to keep them in sync
    void setMasterLoop(int tempo, int measures)
    {
        loopSource.setMasterLoop(tempo, measures);
        repaint();
    }

    int getPosition()
    {
        return loopSource.getPosition();
    }

    //make sure to set this up before calling startRecording()
    void setLastRecording(juce::File file)
    {
        lastRecording = file;
    }

    void setWaitingToRecord(bool newWaitingToRecord)
    {
        waitingToRecord = newWaitingToRecord;
    }

    bool isWaitingToRecord()
    {
        return waitingToRecord;
    }

    //similar to stopRecording, call this after setAsLastRecording to load audio from disk into memory and redraw thumbnail
    //also used when loading a project
    void redrawAndBufferAudio()
    {
        auto file = lastRecording;
        auto reader = formatManager.createReaderFor(file);

        if (reader != nullptr)
        {
            //DN: set up a memory buffer to hold the audio for this loop file
            auto loopBuffer = std::make_unique<juce::AudioBuffer<float>>(reader->numChannels, reader->lengthInSamples);
            //DN: read the audio file into the loopBuffer
            reader->read(loopBuffer.get(), 0, reader->lengthInSamples, 0, true, true);
            //DN: need to delete here because the "createReaderFor" method says to - maybe switch to a unique ptr later for "good practice" reasons!
            delete reader;

            redrawThumbnailWithBuffer(loopBuffer.get());

            // DN: send the loopBuffer object to the loopSource which will handle playback, transfer ownership of unique ptr
            loopSource.setBuffer(loopBuffer.release());
        }
        else
        {
            //if the lastRecording object doesn't exist, we want to reset the loopSource to be blank
            auto loopBuffer = std::make_unique<juce::AudioBuffer<float>>(1, loopSource.getMasterLoopLength());
            for (int channel = 0; channel < loopBuffer->getNumChannels(); ++channel)
            {
                auto writer = loopBuffer->getWritePointer(channel, 0);
                for (int i = 0; i < loopBuffer->getNumSamples(); ++i)
                    writer[i] = 0;  //DN: zero out to avoid pops/clicks
            }

            redrawThumbnailWithBuffer(loopBuffer.get());

            // DN: send the loopBuffer object to the loopSource which will handle playback, transfer ownership of unique ptr
            loopSource.setBuffer(loopBuffer.release());
        }

        repaint();
    }

    //DN: helper function to draw the thumbnail
    void redrawThumbnailWithBuffer(juce::AudioBuffer<float>* loopBuffer)
    {
        //DN: temp const buffer used only for drawing
        juce::AudioBuffer<float> tmpBuffer(loopBuffer->getArrayOfWritePointers(), loopBuffer->getNumChannels(), loopBuffer->getNumSamples());
        //for (int ch = 0; ch < loopBuffer->getNumChannels(); ++ch)
        //    tmpBuffer.copyFrom(ch, 0, loopBuffer.get(), loopBuffer->getNumSamples());
        thumbnail.reset(loopBuffer->getNumChannels(), sampleRate, loopBuffer->getNumSamples());
        thumbnail.addBlock(0, tmpBuffer, 0, loopBuffer->getNumSamples());
    }


    // AF: Listener for changes of values from slider
    // (required by Listener class)
    void sliderValueChanged(juce::Slider* slider) override
    {
        if (slider == &panSlider)
        {
            panSliderValue = slider->getValue();
        }

        if(slider == &gainSlider)
        {
            gainSliderValue = slider->getValue();
        }

        if (slider == &slipController)
        {
            loopSource.setFileStartOffset(slider->getValue());
            repaint();
        }
    }

    /** Called when the button is clicked. */
    void buttonClicked(juce::Button* button)
    {
        if (button == &reverseButton)
        {
            loopSource.reverseAudio();

            //account for slip here?
            redrawThumbnailWithBuffer(loopSource.getLoopBuffer());

            isReversed = !isReversed;
        }
    }

    /** Called when the button's state changes. */
    void buttonStateChanged(juce::Button* button)
    {
    }

    juce::XmlElement* getTrackState(int trackNum)
    {
        juce::String trackElementName = TRACK_FILENAME + juce::String(trackNum);
        // create an inner element..
        juce::XmlElement* trackElement = new juce::XmlElement(trackElementName);

        trackElement->setAttribute("id", trackNum);
        trackElement->setAttribute("pan", panSliderValue);
        trackElement->setAttribute("isReversed", isReversed);
        trackElement->setAttribute("slipValue", slipController.getValue());
        trackElement->setAttribute("gain", gainSlider.getValue());

        return trackElement;
    }

    void restoreTrackState(juce::XmlElement* trackState)
    {
        //set pan
        panSliderValue = trackState->getDoubleAttribute("pan");
        panSlider.setValue(panSliderValue);

        //set gain
        gainSliderValue = trackState->getDoubleAttribute("gain");
        gainSlider.setValue(gainSliderValue);

        //set up slip (needs to happen before reverse)
        const double newSlipValue = trackState->getDoubleAttribute("slipValue");
        slipController.setValue(newSlipValue);
        loopSource.setFileStartOffset(newSlipValue);
        repaint();

        //set up reverse
        isReversed = trackState->getBoolAttribute("isReversed");
        if (isReversed)
        {
            loopSource.reverseAudio();
            redrawThumbnailWithBuffer(loopSource.getLoopBuffer());
        }
    }

    void initializeTrackState()
    {
        panSliderValue = 0.0;
        panSlider.setValue(0.0);
        gainSlider.setValue(1.0);
        isReversed = false;
        slipController.setValue(0.0);
    }

    void mouseEnter(const juce::MouseEvent& event)
    {
        setMouseCursor(juce::MouseCursor::DraggingHandCursor);
    }


    void mouseExit(const juce::MouseEvent& event)
    {
        setMouseCursor(juce::MouseCursor::NormalCursor);
    }

    void mouseDown(const juce::MouseEvent& event)
    {
        dragStart = slipController.getValue();
    }

    void mouseDrag(const juce::MouseEvent& event)
    {
        auto thumbArea = getLocalBounds();
        auto difference = (double)event.getDistanceFromDragStartX()/(double)thumbArea.getWidth() * loopSource.getMasterLoopLength();
        auto newOffset = dragStart + difference;
        slipController.setValue(newOffset);
        repaint();
    }

    void setSettingsHaveBeenOpened(bool newValue)
    {
        settingsHaveBeenOpened = newValue;
        recorder.settingsHaveBeenOpened = newValue;
    }


    juce::Slider panSlider;
    juce::Label panLabel;
    double panSliderValue = 0.0;

    std::unique_ptr<juce::Drawable> reverseSVG;// = juce::Drawable::createFromSVG(*svg_xml_1); // GET THIS AS DRAWABLE
    juce::DrawableButton reverseButton{ "reverseButton",juce::DrawableButton::ButtonStyle::ImageFitted };

    juce::Slider slipController;
    juce::Slider gainSlider;
    double gainSliderValue = 1.0;


    TransportButton recordButton{ "recordButton",MAIN_BACKGROUND_COLOR,MAIN_BACKGROUND_COLOR,MAIN_BACKGROUND_COLOR, TransportButton::TransportButtonRole::Record };


private:

    void timerCallback()
    {
        if (isRecording() && aboutToOverflow)
        {
            DBG("CALLING STOP RECORD");
            stopRecording();
            aboutToOverflow = false;
            sendChangeMessage(); //DN: needed to tell mainComponent we're stopping
        }

        if (waitingToRecord && loopSource.readyToRecord())
        {
           startRecording();
            waitingToRecord = false;
        }

        if (waitingToRecord)
        {
            blinkingCounter++;
            if (blinkingCounter == 50)
                blinkingCounter = 0;

            if (blinkingCounter > 25)
                shouldLightUp = false;
            else
                shouldLightUp = true;

        }
        else if (isRecording())
            shouldLightUp = true;
        else
            shouldLightUp = false;

        if(loopSource.isPlaying()) //DN: added this if so we don't call this when not playing back
            repaint();
    }


    juce::AudioFormatManager formatManager;
    juce::AudioThumbnailCache thumbnailCache{ 10 };
    juce::AudioThumbnail thumbnail{ 512, formatManager, thumbnailCache };

    int samplesPerBlock = 44100;
    int sampleRate = 44100;
    bool aboutToOverflow = false;
    bool isReversed = false;
    bool shouldLightUp = false;
    bool waitingToRecord = false;
    bool settingsHaveBeenOpened = false;
    juce::int64 dragStart = 0;
    int blinkingCounter = 0;

    AudioRecorder recorder{ thumbnail };
    LoopSource loopSource;
    juce::File lastRecording;

    // ---
    bool displayFullThumb = false;

    void changeListenerCallback(juce::ChangeBroadcaster* source) override
    {      
        if (source == &thumbnail)
        {
            repaint();
        }

        //DN: any time any change happens check if we need to turn on/off slip controller
        if (!lastRecording.exists())
        {
            slipController.setEnabled(false);
            slipController.setVisible(false);
        }
        else
        {
            slipController.setEnabled(true);
            slipController.setVisible(true);
        }

        sendSynchronousChangeMessage();
    }
};




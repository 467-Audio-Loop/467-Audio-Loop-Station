/*
  ==============================================================================

    AudioTrack.h
    Created: 26 Jan 2021 3:52:53pm
    Author:  dnego

  ==============================================================================
*/

#pragma once

#include "AudioLiveScrollingDisplay.h"
#include "LoopSource.h"
#include "SaveLoad.h"
#include "customUI.h"




//==============================================================================
/** A simple class that acts as an AudioIODeviceCallback and writes the
    incoming audio data to a WAV file.
*/
class AudioRecorder : public juce::AudioIODeviceCallback, public juce::ChangeBroadcaster
{
public:
    AudioRecorder(juce::AudioThumbnail& thumbnailToUpdate)
        : thumbnail(thumbnailToUpdate)
    {
        backgroundThread.startThread();
        

    }

    ~AudioRecorder() override
    {
        stop();
    }

    //==============================================================================
    void startRecording(const juce::File& file)
    {
        stop();

        if (sampleRate > 0)
        {
            // Create an OutputStream to write to our destination file...
            file.deleteFile();

            if (auto fileStream = std::unique_ptr<juce::FileOutputStream>(file.createOutputStream()))
            {
                // Now create a WAV writer object that writes to our output stream...
                juce::WavAudioFormat wavFormat;

                if (auto writer = wavFormat.createWriterFor(fileStream.get(), sampleRate, inputChannels, 16, {}, 0))
                {
                    fileStream.release(); // (passes responsibility for deleting the stream to the writer object that is now using it)

                    // Now we'll create one of these helper objects which will act as a FIFO buffer, and will
                    // write the data to disk on our background thread.
                    threadedWriter.reset(new juce::AudioFormatWriter::ThreadedWriter(writer, backgroundThread, 32768));

                    // Reset our recording thumbnail
                    thumbnail.reset(writer->getNumChannels(), writer->getSampleRate());
                    nextSampleNum = 0;

                    // And now, swap over our active writer pointer so that the audio callback will start using it..
                    const juce::ScopedLock sl(writerLock);
                    activeWriter = threadedWriter.get();
                }
            }
        }
    }

    void stop()
    {
        // First, clear this pointer to stop the audio callback from using our writer object..
        {
            const juce::ScopedLock sl(writerLock);
            activeWriter = nullptr;
        }

        // Now we can delete the writer object. It's done in this order because the deletion could
        // take a little time while remaining data gets flushed to disk, so it's best to avoid blocking
        // the audio callback while this happens.
        threadedWriter.reset();
    }


    bool isRecording() const
    {
        return activeWriter.load() != nullptr;
    }

    //==============================================================================
    void audioDeviceAboutToStart(juce::AudioIODevice* device) override
    {
        sampleRate = device->getCurrentSampleRate();

        // AF: Get number of input channels
        auto activeInputChannels = device->getActiveInputChannels();
        inputChannels = activeInputChannels.countNumberOfSetBits();

        // AF: Get number of output channels
        auto activeOutputChannels = device->getActiveOutputChannels();
        outputChannels = activeOutputChannels.countNumberOfSetBits();
    }

    void audioDeviceStopped() override
    {
        sampleRate = 0;
    }

    void audioDeviceIOCallback(const float** inputChannelData, int numInputChannels,
        float** outputChannelData, int numOutputChannels,
        int numSamples) override
    {
        const juce::ScopedLock sl(writerLock);

        if (activeWriter.load() != nullptr && numInputChannels >= thumbnail.getNumChannels())
        {
            activeWriter.load()->write(inputChannelData, numSamples);

            // Create an AudioBuffer to wrap our incoming data, note that this does no allocations or copies, it simply references our input data
            juce::AudioBuffer<float> buffer(const_cast<float**> (inputChannelData), thumbnail.getNumChannels(), numSamples);
            thumbnail.addBlock(nextSampleNum, buffer, 0, numSamples);
            nextSampleNum += numSamples;
        }

        // We need to clear the output buffers, in case they're full of junk..
        for (int i = 0; i < numOutputChannels; ++i)
            if (outputChannelData[i] != nullptr)
                juce::FloatVectorOperations::clear(outputChannelData[i], numSamples);
    }

    int getInputChannels() 
    {
        return inputChannels;
    }

    int getOutputChannels()
    {
        return outputChannels;
    }

private:
    juce::AudioThumbnail& thumbnail;
    juce::TimeSliceThread backgroundThread{ "Audio Recorder Thread" }; // the thread that will write our audio data to disk
    std::unique_ptr<juce::AudioFormatWriter::ThreadedWriter> threadedWriter; // the FIFO used to buffer the incoming data
    int inputChannels = 1;
    int outputChannels = 2;
    double sampleRate = 0.0;
    juce::int64 nextSampleNum = 0;

    juce::CriticalSection writerLock;
    std::atomic<juce::AudioFormatWriter::ThreadedWriter*> activeWriter{ nullptr };
};


//==============================================================================
// DN:  I retooled what was the "RecordingThumbail" class, which was just a 
// Component, into an AudioAppComponent that will handle each track's playback

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
       // panLabel.setText("L/R", juce::dontSendNotification);
        //panLabel.attachToComponent(&panSlider, false);

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
        //gainSlider.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 40, 20);

        reverseButton.addListener(this);

        startTimer(10); //used for vertical line position marker
    }



    ~AudioTrack() override
    {
        thumbnail.removeChangeListener(this);
        deviceManager.removeAudioCallback(&recorder);
    }

    //DN:: added along with AudioDeviceIOCallback to get mainComponent to tell this which tells the recorder when to start
    // sending audio data
    void audioDeviceIOCallback(const float** inputChannelData,
        int numInputChannels,
        float** outputChannelData,
        int numOutputChannels,
        int numSamples) override
    {
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

    //DN:  we don't need this anymore, using the thumbnail within the class
    //juce::AudioThumbnail& getAudioThumbnail() { return thumbnail; }

    void setDisplayFullThumbnail(bool displayFull)
    {
        displayFullThumb = displayFull;
        repaint();
    }
    
    void paint(juce::Graphics& g) override
    {
        auto thumbnailBorder = 8;
        g.fillAll(MAIN_BACKGROUND_COLOR);
        g.setColour(MAIN_DRAW_COLOR);
        const juce::Rectangle<float> area(getLocalBounds().reduced(thumbnailBorder).toFloat());
        g.drawRoundedRectangle(area, ROUNDED_CORNER_SIZE, THIN_LINE);

        if (thumbnail.getTotalLength() > 0.0)
        {
            //auto endTime = displayFullThumb ? thumbnail.getTotalLength()
            //    : juce::jmax(30.0, thumbnail.getTotalLength());

            //DN:  paint the thumbnail audio horizontally relative to master loop and the slip offset
            auto startTime = -(double)slipController.getValue() / sampleRate;
            auto endTime = (double)(loopSource.getMasterLoopLength() / sampleRate) + startTime;

            auto thumbArea = getLocalBounds().reduced(thumbnailBorder);
            
            thumbnail.drawChannels(g, thumbArea, startTime, endTime, 1.0f); // 1.0f is zoom


            //DN: paint vertical line to indicate playhead position
            g.setColour(MAIN_DRAW_COLOR);
            auto audioPosition = (float)loopSource.getPosition();
            auto drawPosition = (audioPosition / loopSource.getMasterLoopLength()) * (float)thumbArea.getWidth() + (float)thumbArea.getX();            
            g.drawLine(drawPosition, (float)thumbArea.getY()+8, drawPosition, (float)thumbArea.getBottom()-8, 2.0f);                       

           // redrawThumbnailWithBuffer(loopSource.getLoopBuffer());
        }
        else
        {
            //g.setFont(14.0f);
           // g.drawFittedText("(No file recorded)", getLocalBounds(), juce::Justification::centred, 2);
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

        // AF: This essentially halts this code until loopSource is ready to record (pos == 0)
        while (!loopSource.readyToRecord())
        {
            continue;
        }

        loopSource.setBeginningOfFile(false);

        //DN:  tell loopSource to play silence/not access loopBuffer while we switch it out
        loopSource.startRecording(); 
       
        recorder.startRecording(lastRecording);

        setDisplayFullThumbnail(false);
    }

    void stopRecording()
    {
        recorder.stop();

       
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

    // DN:  set position in samples now, not seconds anymore (made it an int, not a double)
    void setPosition(juce::int64 newPosition)
    {
        loopSource.setNextReadPosition(newPosition);
    }

    void start()
    {
        loopSource.start(0);
    }

    void stop()
    {
        loopSource.stop();
    }

    //Call this for all tracks to keep them in sync
    void setMasterLoop(int tempo, int measures, int beatsPerMeasure)
    {
        loopSource.setMasterLoop(tempo, measures, beatsPerMeasure);
    }

    // AF: Same function from LoopSource.h in order to access it from MainComponent.cpp
    void setFileStartOffset(int newStartOffset)
    {
        loopSource.setFileStartOffset(newStartOffset);
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

            //DN: account for reverse
            if (isReversed)
            {
                loopBuffer->reverse(0, loopBuffer->getNumSamples());
            }

            redrawThumbnailWithBuffer(loopBuffer.get());

            // DN: send the loopBuffer object to the loopSource which will handle playback, transfer ownership of unique ptr
            loopSource.setBuffer(loopBuffer.release());
        }
        else
        {
            //if the lastRecording object doesn't exist, we want to reset the loopSource to be blank
            auto loopBuffer = std::make_unique<juce::AudioBuffer<float>>(2, 0);

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
            redrawThumbnailWithBuffer(loopSource.getLoopBuffer());             //account for slip here
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


    // AF: Slider for panning
    juce::Slider panSlider;
    juce::Label panLabel;
    double panSliderValue = 0.0;

    juce::TextButton reverseButton{ "Reverse" };
    juce::Slider slipController;
    juce::Slider gainSlider;
    double gainSliderValue = 1.0;

    //juce::TextButton recordButton{ "Record" };
    TransportButton recordButton{ "recordButton",MAIN_BACKGROUND_COLOR,MAIN_BACKGROUND_COLOR,MAIN_BACKGROUND_COLOR, TransportButton::TransportButtonRole::Record };
private:

    void timerCallback()
    {
        repaint();
    }


    juce::AudioFormatManager formatManager;
    juce::AudioThumbnailCache thumbnailCache{ 10 };
    juce::AudioThumbnail thumbnail{ 512, formatManager, thumbnailCache };

    int samplesPerBlock = 44100;
    int sampleRate = 44100;
    bool aboutToOverflow = false;
    bool isReversed = false;
    juce::int64 dragStart = 0;

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
            
        if (aboutToOverflow)
        {
            stopRecording();
            aboutToOverflow = false;
        }

        // AF: Stop recording once loop reaches max length
        //if (source == &thumbnail && (loopSource.getMasterLoopLength() - loopSource.getPosition() < samplesPerBlock))
        if (source == &thumbnail && (loopSource.getPosition() + samplesPerBlock >= loopSource.getMasterLoopLength()))
        {
            //stopRecording();
            //repaint();
            aboutToOverflow = true;
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




//DN:  A simple AudioSource class where we can send the audio input in buffer form, to be read from by our main mixer
class InputMonitor : public juce::AudioSource
{
public:
    InputMonitor()
    {
        inputBuffer.reset(new juce::AudioBuffer<float>(2, 0));
    }
    ~InputMonitor()
    {

    }
    void prepareToPlay(int samplesPerBlockExpected, double sampleRate)
    {
        
    }
    void releaseResources()
    {

    }
    void getNextAudioBlock(const juce::AudioSourceChannelInfo& bufferToFill) 
    {
        

        int loopBufferSize = inputBuffer->getNumSamples();
        int maxInChannels =  inputBuffer->getNumChannels();

        int maxOutChannels = bufferToFill.buffer->getNumChannels();

        if (loopBufferSize > 0 && maxInChannels > 0)
        {
            for (int i = 0; i < maxOutChannels; ++i)
            {
                auto writer = bufferToFill.buffer->getWritePointer(i, bufferToFill.startSample);

                for (auto sample = 0; sample < bufferToFill.numSamples; ++sample)
                {
                    writer[sample] = inputBuffer->getSample(i % maxInChannels, juce::jmin(sample, loopBufferSize)) * gain;
                }
            }
        }

    }

    void setBuffer(juce::AudioSampleBuffer* newBuffer)
    {
        inputBuffer.reset(newBuffer);
    }

    void setGain(double newGain)
    {
        gain = newGain;
    }

private:
    std::unique_ptr<juce::AudioBuffer<float>> inputBuffer;
    double gain = 1.0;

};


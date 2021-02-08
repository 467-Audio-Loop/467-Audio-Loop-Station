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

                if (auto writer = wavFormat.createWriterFor(fileStream.get(), sampleRate, 1, 16, {}, 0))
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

private:
    juce::AudioThumbnail& thumbnail;
    juce::TimeSliceThread backgroundThread{ "Audio Recorder Thread" }; // the thread that will write our audio data to disk
    std::unique_ptr<juce::AudioFormatWriter::ThreadedWriter> threadedWriter; // the FIFO used to buffer the incoming data
    double sampleRate = 0.0;
    juce::int64 nextSampleNum = 0;

    juce::CriticalSection writerLock;
    std::atomic<juce::AudioFormatWriter::ThreadedWriter*> activeWriter{ nullptr };
};


//==============================================================================
// DN:  I retooled what was the "RecordingThumbail" class, which was just a 
// Component, into an AudioAppComponent that will handle each track's playback

class AudioTrack : public juce::AudioAppComponent,
    private juce::ChangeListener, public juce::ChangeBroadcaster, public juce::AudioIODeviceCallback
{
public:
    AudioTrack(juce::String desiredFilename)
    {
        formatManager.registerBasicFormats();
        thumbnail.addChangeListener(this);
        loopSource.addChangeListener(this);
        deviceManager.addAudioCallback(&recorder);

        //DN: adapted from old "recordingSaved()" function, we now check if the file already exists and 
        //just use it if it does.  Otherwise make it.
#if (JUCE_ANDROID || JUCE_IOS)
        auto parentDir = juce::File::getSpecialLocation(juce::File::tempDirectory);
#else
        // AF: Here it seems the user's "Documents" path is stored in parentDir
        auto parentDir = juce::File::getSpecialLocation(juce::File::userDocumentsDirectory);
#endif
        lastRecording = parentDir.getChildFile(desiredFilename);
        if (!lastRecording.existsAsFile())
        {
            lastRecording = parentDir.getNonexistentChildFile(desiredFilename, ".wav"); 
        }
        else
        {
            // this will draw the recordings from last time when opening the app
            
            repaint();
        }
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
        g.fillAll(juce::Colours::darkslategrey);
        g.setColour(juce::Colours::cyan);

        if (thumbnail.getTotalLength() > 0.0)
        {
            auto endTime = displayFullThumb ? thumbnail.getTotalLength()
                : juce::jmax(30.0, thumbnail.getTotalLength());

            auto thumbArea = getLocalBounds();
            thumbnail.drawChannels(g, thumbArea.reduced(2), 0.0, endTime, 1.0f);
        }
        else
        {
            g.setFont(14.0f);
           // g.drawFittedText("(No file recorded)", getLocalBounds(), juce::Justification::centred, 2);
        }
    }

    //DN:  new functions needed
    //==============================================================================
    void prepareToPlay(int samplesPerBlockExpected, double sampleRate) override 
    {
        loopSource.prepareToPlay(samplesPerBlockExpected, sampleRate);
    }

    void getNextAudioBlock(const juce::AudioSourceChannelInfo& bufferToFill) override 
    {
        loopSource.getNextAudioBlock(bufferToFill);
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
            juce::AudioBuffer<float>* loopBuffer = new juce::AudioBuffer<float>(reader->numChannels, reader->lengthInSamples);
            //DN: read the audio file into the loopBuffer
            reader->read(loopBuffer, 0, reader->lengthInSamples, 0, true, true);
            //DN: need to delete here because the "createReaderFor" method says to - maybe switch to a unique ptr later for "good practice" reasons!
            delete reader; 
            // DN: send the loopBuffer object to the loopSource which will handle playback
            loopSource.setBuffer(loopBuffer);

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
        loopSource.start();
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


private:
    juce::AudioFormatManager formatManager;
    juce::AudioThumbnailCache thumbnailCache{ 10 };
    juce::AudioThumbnail thumbnail{ 512, formatManager, thumbnailCache };

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
            
        // AF: Stop recording once loop reaches max length
        // This does not seem to work because for some reason the position does not get updated while it's recording
        if (isRecording())
        {
            if (loopSource.getPosition() == loopSource.getMasterLoopLength())
                stopRecording();
        }

        sendSynchronousChangeMessage();
    }
};


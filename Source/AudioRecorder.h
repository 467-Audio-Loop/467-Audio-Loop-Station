/*
  ==============================================================================

    AudioRecorder.h

    A simple class that acts as an AudioIODeviceCallback and writes the
    incoming audio data to a WAV file.

    Borrowed from JUCE's Audio Recording Demo, with some mild tweaks

  ==============================================================================
*/

#pragma once


#include <JuceHeader.h>


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

                    if (writer->getNumChannels() != 0)
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

        if (!settingsHaveBeenOpened && inputChannels > 1)
            inputChannels = 1;

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

    bool settingsHaveBeenOpened = false;

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


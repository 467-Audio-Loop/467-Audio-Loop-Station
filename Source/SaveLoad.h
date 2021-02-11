/*
  ==============================================================================

    SaveLoad.h
    Created: 10 Feb 2021 11:52:00am
    Author:  dnego

    This is where we'll handle creating and navigating the directory structure 
    where we'll save and load our loop files
  ==============================================================================
*/

#pragma once


#define MASTER_FOLDER_NAME "467Audio"
#define SAVED_LOOPS_FOLDER_NAME "Saved Loops"
#define TEMP_LOOP_FOLDER_NAME "Temp WAVs"
#define TRACK1_FILENAME "Loopstation Track 1.wav"
#define TRACK2_FILENAME "Loopstation Track 2.wav"
#define TRACK3_FILENAME "Loopstation Track 3.wav"
#define TRACK4_FILENAME "Loopstation Track 4.wav"


class DirectoryTree
{
public:
    DirectoryTree()
    {
#if (JUCE_ANDROID || JUCE_IOS)
        auto parentDir = juce::File::getSpecialLocation(juce::File::tempDirectory);
#else
        // AF: Here it seems the user's "Documents" path is stored in parentDir
        auto parentDir = juce::File::getSpecialLocation(juce::File::userDocumentsDirectory);
#endif

        //DN: set up our folder hierarchy for saving/loading loop wavs in different folders
        masterFolder = parentDir.getChildFile(MASTER_FOLDER_NAME);
        if (!masterFolder.exists())
            masterFolder.createDirectory();

        tempLoopFolder = masterFolder.getChildFile(TEMP_LOOP_FOLDER_NAME);
        if (!tempLoopFolder.exists())
            tempLoopFolder.createDirectory();

        savedLoopsFolder = masterFolder.getChildFile(SAVED_LOOPS_FOLDER_NAME);
        if (!savedLoopsFolder.exists())
            savedLoopsFolder.createDirectory();

        

    }
    
    ~DirectoryTree()
    {

    }

    //DN: We check if this track wav file exists in the current loop dir and make it if it doesn't
    juce::File getOrCreateWAVInTempLoopDir(juce::String wavFilename)
    {
        auto wavFile = tempLoopFolder.getChildFile(wavFilename);
        if (!wavFile.existsAsFile())
            wavFile = tempLoopFolder.getNonexistentChildFile(wavFilename, ".wav");

        return wavFile;
    }

    juce::StringArray getLoopFolderNamesArray()
    {
        juce::StringArray loopFolderNamesArray;

        juce::Array<juce::File> childDirs = savedLoopsFolder.findChildFiles(1, false);
        for (juce::File file : childDirs)
            loopFolderNamesArray.add(file.getFileName());

        return loopFolderNamesArray;
    }

    //DN:  retrives saved audio files from given folder name, copies them to temp loop folder 
    //  so they can be loaded into the project, then potentially recorded over
    // without affecting the original saved audio
    //call this when loading a previously saved project
    //returns true if successful, false otherwise
    bool loadWAVsFrom(juce::String folderName)
    {
        auto folderToCopyFrom = savedLoopsFolder.getChildFile(folderName);
        if (!folderToCopyFrom.exists())
            return false;

        juce::Array<juce::File> childWAVs = folderToCopyFrom.findChildFiles(2, false);
        for (juce::File fileToCopy : childWAVs)
        {
            //DN: the track WAV filenames will be the same for all sets of loops.  Folder names differentiate the loops
            juce::File destinationFile = tempLoopFolder.getChildFile(fileToCopy.getFileName());
            if (!fileToCopy.copyFileTo(destinationFile))
                return false;
        }

        return true;
            
    }



    bool saveWAVsTo(juce::String folderName)
    {
        auto folderToCopyTo = savedLoopsFolder.getChildFile(folderName);
        if (!folderToCopyTo.exists())
            return false;

        juce::Array<juce::File> childWAVs = folderToCopyTo.findChildFiles(2, false);
        for (juce::File fileToReplace : childWAVs)
        {
            //DN: the track WAV filenames will be the same for all sets of loops.  Folder names differentiate the loops
            juce::File sourceFile = tempLoopFolder.getChildFile(fileToReplace.getFileName());
            if (!sourceFile.copyFileTo(fileToReplace))
                return false;
        }

        return true;

    }


private:
    juce::File masterFolder;
    juce::File tempLoopFolder;
    juce::File savedLoopsFolder;
};



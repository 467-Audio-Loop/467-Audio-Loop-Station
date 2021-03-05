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


#define MASTER_FOLDER_NAME "Loopspace"
#define SAVED_LOOPS_FOLDER_NAME "Saved Loops"
#define TEMP_LOOP_FOLDER_NAME "Temp WAVs"
#define TRACK_FILENAME "LoopspaceTrack"
#define NUM_TRACKS  4
#define PROJECT_STATE_XML_FILENAME "projectState.xml"


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

    //DN: We delete the file if in exists in temp loop dir and return
    //a blank file object to be used for a fresh project
    juce::File setFreshWAVInTempLoopDir(juce::String wavFilename)
    {
        auto wavFile = tempLoopFolder.getChildFile(wavFilename + ".wav");
        if (wavFile.existsAsFile())
            wavFile.deleteFile();
        wavFile = tempLoopFolder.getNonexistentChildFile(wavFilename, ".wav");

        return wavFile;
    }

    //DN: We return the file object for the filename in temp loop dir
    //if it exists, the audio on the file will be used
    juce::File getOrCreateWAVInTempLoopDir(juce::String wavFilename)
    {
        auto wavFile = tempLoopFolder.getChildFile(wavFilename + ".wav");
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

        //DN: delete old WAVs to be overwritten
        juce::Array<juce::File> wavsToDelete = tempLoopFolder.findChildFiles(2, false);
        for (juce::File fileToDelete : wavsToDelete)
            fileToDelete.deleteFile();

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


    //DN: copies current WAVs from temp folder to designated folder in Saved Loops directory.
    //  if the folder name doesn't exist it will be created
    //if WAVs exist in the folder already they will be overwritten
    bool saveWAVsTo(juce::String folderName)
    {
        auto folderToCopyTo = savedLoopsFolder.getChildFile(folderName);
        if (!folderToCopyTo.exists())
        {
            folderToCopyTo.createDirectory();

        }
 
        //DN: delete old WAVs to be overwritten
        juce::Array<juce::File> wavsToDelete = folderToCopyTo.findChildFiles(2, false);
        for (juce::File fileToDelete : wavsToDelete)
            fileToDelete.deleteFile();

        //DN: get WAVs to copy
        juce::Array<juce::File> wavsToCopy = tempLoopFolder.findChildFiles(2, false);

        for (juce::File fileToCopy : wavsToCopy)
        {
            juce::File destFile = folderToCopyTo.getChildFile(fileToCopy.getFileName());
            if (!fileToCopy.copyFileTo(destFile))
                return false;
        }

        return true;

    }

    juce::File getProjectFolder(juce::String folderName)
    {
        return savedLoopsFolder.getChildFile(folderName);
    }


private:
    juce::File masterFolder;
    juce::File tempLoopFolder;
    juce::File savedLoopsFolder;
};



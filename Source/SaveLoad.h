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



#define MASTER_LOOP_FOLDER_NAME "467Audio Loops"
#define TEMP_LOOP_FOLDER_NAME "Default"
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
        masterLoopsFolder = parentDir.getChildFile(MASTER_LOOP_FOLDER_NAME);
        if (!masterLoopsFolder.exists())
            masterLoopsFolder.createDirectory();

        defaultLoopFolder = masterLoopsFolder.getChildFile(TEMP_LOOP_FOLDER_NAME);
        if (!defaultLoopFolder.exists())
            defaultLoopFolder.createDirectory();

        //initially we are working in the default dir
        currentLoopFolder = defaultLoopFolder;

        

    }
    
    ~DirectoryTree()
    {

    }

    //DN: We check if this track wav file exists in the current loop dir and make it if it doesn't
    juce::File getOrCreateWAVInCurrentLoopDir(juce::String wavFilename)
    {
        auto wavFile = currentLoopFolder.getChildFile(wavFilename);
        if (!wavFile.existsAsFile())
            wavFile = currentLoopFolder.getNonexistentChildFile(wavFilename, ".wav");

        return wavFile;
    }

    juce::StringArray getLoopFolderNamesArray()
    {
        juce::StringArray loopFolderNamesArray;

        //const juce::File dirToSearch = masterLoopsFolder;
        //for (juce::DirectoryEntry entry : juce::RangedDirectoryIterator(dirToSearch, false,"*",0))
        //{
        //    auto file = entry.getFile();
        //    auto string = file.getFileName();
        //    loopFolderArray.add(string);
        //}

        juce::Array<juce::File> childDirs = masterLoopsFolder.findChildFiles(1, false);
        for (juce::File file : childDirs)
            loopFolderNamesArray.add(file.getFileName());

        return loopFolderNamesArray;
    }


    //DN: this method sets the folder we will be recording our audio file to
//  if it doesn't exist it will be created
    void createAndSetCurrentLoopFolder(juce::String folderName)
    {
        currentLoopFolder = masterLoopsFolder.getChildFile(folderName);
        if (!currentLoopFolder.exists())
            currentLoopFolder.createDirectory();
        else
        {
            //DN:  need to add some kind of warning here that that loop exists already
            //maybe make return type a bool
        }

    }



private:
    juce::File masterLoopsFolder;
    juce::File currentLoopFolder;
    juce::File defaultLoopFolder;
};



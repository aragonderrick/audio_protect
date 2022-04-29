#pragma once

#include <JuceHeader.h>
#include "Range.h"
#include "hashTable.h"
#include <algorithm>
#include <vector>
#include <string>

using Range = juce::NormalisableRange<float>;

class MainComponent  : public juce::AudioAppComponent {
public:
    enum
    {
        fftOrder = 13,
        fftSize = 1 << fftOrder
    };

    //==============================================================================
    MainComponent();
    ~MainComponent() override;
    //==============================================================================
    void prepareToPlay(int samplesPerBlockExpected, double sampleRate) override;
    void getNextAudioBlock(const juce::AudioSourceChannelInfo& bufferToFill) override;
    void releaseResources() override;
    void paint (juce::Graphics& g) override;
    void resized() override;
    //==============================================================================
    void openButtonClicked();
    void pushNextSampleIntoFifo(float sample) noexcept;
    void drawNextLineOfSpectrogram();
    void readInFileFFT(const juce::File& file);
    void drawSpectrogram();
    void drawConstellationImage();
    void generateFingerprint();
    void populateFingerprints();

private:
    // Buttons
    juce::TextButton openButton;

    // Format and Audio Variables
    juce::AudioFormatManager formatManager; // takes care of audio formatting
    std::unique_ptr< juce::AudioFormatReaderSource> readerSource; // source to read audio data from
    juce::AudioTransportSource transportSource; // allows audio to be played, stopped, etc
    std::unique_ptr<juce::FileChooser> chooser;
    juce::AudioSampleBuffer fileBuffer; // stores the data from the file

    // Objects and variables for spectrogram
    juce::dsp::FFT fft;
    juce::Image spectrogramImage;
    juce::Image constellationImage;
    juce::Image combinedImage;

    // Objects and variables for hashtable
    HashTable hashtable;

    // variables needed for FFT
    std::array<float, fftSize> fifo;
    std::array<float, fftSize*2> fftData;
    std::vector<std::vector<std::pair<float, int>>> constellationData; // vector of pairs where <fftData, y-axis pixel>
    std::vector<std::vector<std::pair<int, int>>> hashingData; // vector of pairs where <fftData (floor), y-axis pixel>
    int fifoIndex = 0;
    bool nextFFTBlockReady = false;
    float maxValue;
    int position; //position in the array of sample data
    int pixelX;
    int fftIndex;

    // Other variables required (non-specific to a certain portion of the algorithm)
    double duration;
    bool draw;
    Range normalRange;
    juce::String currentSizeAsString;
    std::string currentStatus;
    std::string song_name;
    juce::Image logo = juce::ImageFileFormat::loadFrom(juce::File("C:/Users/arago/OneDrive/Desktop/Spring2022/CSCI490/images/logo2.png"));
   
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MainComponent)
};

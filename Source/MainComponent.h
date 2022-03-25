#pragma once

#include <JuceHeader.h>

class MainComponent  : public juce::AudioAppComponent, private juce::Timer
{
public:
    enum
    {
        fftOrder = 9,
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
    void timerCallback() override; //repaints at 60 hz when playing the wav file
    void pushNextSampleIntoFifo(float sample) noexcept;
    void drawNextLineOfSpectrogram();
    void readInFileFFT(const juce::File& file);
    void drawSpectrogram();

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

    // variables needed for FFT
    std::array<float, fftSize> fifo;
    std::array<float, fftSize*2> fftData;
    //float fifo[fftSize];
    //float fftData[2 * fftSize];
    int fifoIndex = 0;
    bool nextFFTBlockReady = false;
    float maxValue;
    float** channels;
    int position; //position in the array of sample data
    juce::String currentSizeAsString;
    juce::String currentStatus;
    juce::Image logo = juce::ImageFileFormat::loadFrom(juce::File("C:/Users/arago/OneDrive/Desktop/Spring2022/CSCI490/images/logo2.png"));
   
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MainComponent)
};

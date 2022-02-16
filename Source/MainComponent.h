#pragma once

#include <JuceHeader.h>

class MainComponent  : public juce::AudioAppComponent, public juce::ChangeListener
{
public:
    // Enumerations
    enum TransportState
    {
        Stopped,    //Audio playback is stopped and ready to be started.
        Starting,   //Audio playback hasn't yet started but it has been told to start.
        Pausing,    //Audio is pausing.
        Paused,     //Audio playback is paused and ready to be started.
        Playing,    //Audio is playing.
        Stopping    //Audio is playing but playback has been told to stop, after this it will return to the Stopped state.
    };
    //==============================================================================
    MainComponent();
    ~MainComponent() override;

    //==============================================================================
    void prepareToPlay (int samplesPerBlockExpected, double sampleRate) override;
    void getNextAudioBlock (const juce::AudioSourceChannelInfo& bufferToFill) override;
    void releaseResources() override;
    void openButtonClicked();
    void playButtonClicked();
    void stopButtonClicked();
    void changeListenerCallback(juce::ChangeBroadcaster* source) override;
    void changeState(TransportState newState);

    //==============================================================================
    void paint (juce::Graphics& g) override;
    void resized() override;

private:
    // Buttons
    juce::TextButton openButton;
    juce::TextButton playButton;
    juce::TextButton stopButton;
    
    // Format and Audio Variables
    juce::AudioFormatManager formatManager;
    std::unique_ptr< juce::AudioFormatReaderSource> readerSource;
    juce::AudioTransportSource transportSource;
    TransportState current_state;
    std::unique_ptr<juce::FileChooser> chooser;
    
    // Other 
    juce::String currentSizeAsString;
    //juce::VideoComponent spectrogram_no_constelation;
    //juce::VideoComponent spectrogram_with_constelation;
    //juce::VideoComponent just_constelation;
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MainComponent)
};

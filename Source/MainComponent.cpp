#include "MainComponent.h"
//==============================================================================
MainComponent::MainComponent()
    : current_state(Stopped), openButton("Select a File..."), playButton("Play"), stopButton("Stop")
{
    //addAndMakeVisible(spectrogram_no_constelation);
    //addAndMakeVisible(spectrogram_with_constelation);
    //addAndMakeVisible(just_constelation);
  
    // Buttons
    addAndMakeVisible(&openButton);
    openButton.onClick = [this] { openButtonClicked(); };

    addAndMakeVisible(&playButton);
    playButton.onClick = [this] { playButtonClicked(); };
    playButton.setEnabled(false);

    addAndMakeVisible(&stopButton);
    stopButton.onClick = [this] { stopButtonClicked(); };
    stopButton.setEnabled(false);


    formatManager.registerBasicFormats(); // allows for .wav and .aaif files
    transportSource.addChangeListener(this);  //respond to changes in state

    setSize (500, 200);

    // Some platforms require permissions to open input channels so request that here
    if (juce::RuntimePermissions::isRequired (juce::RuntimePermissions::recordAudio)
        && ! juce::RuntimePermissions::isGranted (juce::RuntimePermissions::recordAudio))
    {
        juce::RuntimePermissions::request (juce::RuntimePermissions::recordAudio,
                                           [&] (bool granted) { setAudioChannels (granted ? 2 : 0, 2); });
    }
    else
    {
        // Specify the number of input and output channels that we want to open
        setAudioChannels (2, 2);
    }
}

MainComponent::~MainComponent() {
    // This shuts down the audio device and clears the audio source.
    shutdownAudio();
}

//==============================================================================
void MainComponent::prepareToPlay (int samplesPerBlockExpected, double sampleRate) {
    transportSource.prepareToPlay(samplesPerBlockExpected, sampleRate);
}

void MainComponent::getNextAudioBlock (const juce::AudioSourceChannelInfo& bufferToFill) {
    if (readerSource.get() == nullptr)
    {
        bufferToFill.clearActiveBufferRegion();
        return;
    }
    transportSource.getNextAudioBlock(bufferToFill);
}

void MainComponent::releaseResources() {
    transportSource.releaseResources();
}

//==============================================================================
void MainComponent::paint (juce::Graphics& g) {
    // (Our component is opaque, so we must completely fill the background with a solid colour)
    g.fillAll (getLookAndFeel().findColour (juce::ResizableWindow::backgroundColourId));
    //auto& random = juce::Random::getSystemRandom();
    //juce::Colour currentBackgroundColour(random.nextInt(256), random.nextInt(256), random.nextInt(256));
    //g.fillAll(currentBackgroundColour);
    //g.fillAll(juce::Colours::skyblue);
    g.setFont(40.0f);
    g.setColour(juce::Colours::white);
    //g.drawText("Hello Juce!", getLocalBounds(), juce::Justification::centredTop, true);
    g.setFont(20.0f);
    g.drawText(currentSizeAsString, getLocalBounds(), juce::Justification::bottomRight, true);

    // You can add your drawing code here!
}

void MainComponent::resized() {
    //spectrogram_no_constelation.setBounds(getLocalBounds());
    //spectrogram_with_constelation.setBounds(getLocalBounds());
    //just_constelation.setBounds(getLocalBounds());
    openButton.setBounds(10, 10, getWidth() - 20, 30);
    playButton.setBounds(10, 50, getWidth() - 20, 30);
    stopButton.setBounds(10, 90, getWidth() - 20, 30);
    currentSizeAsString = juce::String(getWidth()) + " x " + juce::String(getHeight());
}

void MainComponent::openButtonClicked() {
    //juce::FileChooser file_chooser("Select a WAV file",
    //    juce::File::getSpecialLocation(juce::File::userDesktopDirectory),
    //    "*wav");
    //Create the FileChooser object with a short message and allow the user to select only .wav files
    chooser = std::make_unique<juce::FileChooser>("Select a Wave file to play...", juce::File{}, "*.wav");
    // Pop up the FileChooser object
    auto chooserFlags = juce::FileBrowserComponent::openMode | juce::FileBrowserComponent::canSelectFiles;
    chooser->launchAsync(chooserFlags, [this](const juce::FileChooser& fc) {
        auto file = fc.getResult();
        // if () will succeed if the user actually selects a file(rather than cancelling)
        if (file != juce::File{}) {
            // attempt to create a reader for this particular file. 
            // This will return the nullptr value if it fails
            auto* reader = formatManager.createReaderFor(file);

            if (reader != nullptr) {
                // create a new AudioFormatReaderSource object using the reader we just created
                // We store the AudioFormatReaderSource object in a temporary std::unique_ptr 
                // object to avoid deleting a previously allocated AudioFormatReaderSource prematurely 
                // on subsequent commands to open a file.
                auto newSource = std::make_unique<juce::AudioFormatReaderSource>(reader, true);
                // The AudioFormatReaderSource object is connected to our AudioTransportSource object that is being used in our getNextAudioBlock()
                transportSource.setSource(newSource.get(), 0, nullptr, reader->sampleRate);
                playButton.setEnabled(true);
                // we can safely store the AudioFormatReaderSource object in our readerSource member
                // we must transfer ownership from the local newSource variable by using std::unique_ptr::release()
                readerSource.reset(newSource.release());
            }
        }
    });
}// openButtonClicked()

void MainComponent::playButtonClicked() {
    if ((current_state == Stopped) || (current_state == Paused)) {
        changeState(Starting);
    }
    else if (current_state == Playing) {
        changeState(Pausing);
    }
}// playButtonClicked()

void MainComponent::stopButtonClicked() {
    if (current_state == Paused) {
        changeState(Stopped);
    }
    else {
        changeState(Stopping);
    }
}// stopButtonClicked()

void MainComponent::changeListenerCallback(juce::ChangeBroadcaster* source) {
    if (source == &transportSource)
    {
        if (transportSource.isPlaying())
            changeState(Playing);
        else if ((current_state == Stopping) || (current_state == Playing))
            changeState(Stopped);
        else if (Pausing == current_state)
            changeState(Paused);
    }
}

void MainComponent::changeState(TransportState newState) {
    if (current_state != newState)
    {
        current_state = newState;

        switch (current_state)
        {
        case Stopped:
            /*
                transport returns to the Stopped state it disables the 
                Stop button, enables the Play button, and resets the transport 
                position back to the start of the file.
            */
            playButton.setButtonText("Play");
            stopButton.setButtonText("Stop");
            stopButton.setEnabled(false);
            transportSource.setPosition(0.0);
            break;

        case Starting:
            /*
                Starting state is triggered by the user clicking the Play button, 
                this tells the AudioTransportSource object to start playing. 
                At this point we disable the Play button
            */
            transportSource.start();
            break;

        case Pausing:
            /*
                Pausing state is triggered by the user clicking the pause button,
                this tells the AudioTransportSource object to pause playing.
            */
            transportSource.stop();
            break;

        case Paused:
            /*
                transport goes to the paused state it renames the play button,
                and offeres user to resets the transport
                position back to the start of the file.
            */
            playButton.setButtonText("Resume");
            stopButton.setButtonText("Return to Start of Audio");
            break;

        case Playing:
            /*
                Playing state is triggered by the AudioTransportSource object 
                reporting a change via the changeListenerCallback() function. 
                Here we enable the Stop button.
            */
            playButton.setButtonText("Pause");
            stopButton.setButtonText("Stop");
            stopButton.setEnabled(true);
            break;

        case Stopping:
            /*
                Stopping state is triggered by the user clicking the Stop button, 
                so we tell the AudioTransportSource object to stop.
            */
            transportSource.stop();
            break;
        }
    }
}
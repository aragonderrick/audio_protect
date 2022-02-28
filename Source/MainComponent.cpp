#include "MainComponent.h"
//==============================================================================
MainComponent::MainComponent()
    : current_state(Stopped), 
    openButton("Select a File..."), 
    playButton("Play"), 
    stopButton("Stop"), 
    fft(fftOrder), 
    spectrogramImage(juce::Image::RGB, 660, 330, true), 
    constellationImage(juce::Image::RGB, 660, 330, true), 
    combinedImage(juce::Image::RGB, 1360, 330, true),
    currentStatus("Please Select a Video for Processing"),
    maxValue(1)
    //sampleRate(44100) //sample rate (defaults to 44100, is updated later by reader)
{
    // Buttons
    addAndMakeVisible(&openButton);
    openButton.onClick = [this] { openButtonClicked(); };

    addAndMakeVisible(&playButton);
    playButton.onClick = [this] { playButtonClicked(); };
    playButton.setEnabled(false);

    addAndMakeVisible(&stopButton);
    stopButton.onClick = [this] { stopButtonClicked(); };
    stopButton.setEnabled(false);

    setOpaque(true);
    startTimerHz(60);

    setSize (1400, 900);

    formatManager.registerBasicFormats(); // allows for .wav and .aaif files
    transportSource.addChangeListener(this);  //respond to changes in state

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
    // fill the buffer via the transport source
    transportSource.getNextAudioBlock(bufferToFill);

    if (bufferToFill.buffer->getNumChannels() > 0)
    {
        auto* channelData = bufferToFill.buffer->getReadPointer(0, bufferToFill.startSample);

        for (auto i = 0; i < bufferToFill.numSamples; ++i)
            pushNextSampleIntoFifo(channelData[i]);
    }
}

void MainComponent::releaseResources() {
    transportSource.releaseResources();
}

//==============================================================================
void MainComponent::paint (juce::Graphics& g) {
    g.fillAll (getLookAndFeel().findColour (juce::ResizableWindow::backgroundColourId));
    g.setColour(juce::Colours::white);
    g.drawImageAt(spectrogramImage, 20, 20);
    g.drawImageAt(constellationImage, (getWidth() / 2)+20, 20);
    g.drawImageAt(combinedImage, 20, 380);
    g.drawImageAt(logo, (getWidth() / 2)-160, 750);
    g.setFont(20.0f);
    g.drawMultiLineText(currentStatus, (getWidth()-400), 800, 400);
    g.drawText(currentSizeAsString, getLocalBounds(), juce::Justification::bottomRight, true);
}


void MainComponent::resized() {
    openButton.setBounds(100, 740, ((getWidth() - 20)/2)-320, 30);
    playButton.setBounds(100, 780, ((getWidth() - 20) / 2)-320, 30);
    stopButton.setBounds(100, 820, ((getWidth() - 20) / 2)-320, 30);
    currentSizeAsString = juce::String(getWidth()) + " x " + juce::String(getHeight());
}


//repaints at 60 hz when playing the wav file
void MainComponent::timerCallback() {
    if (current_state == Playing) {
        repaint();
    }
}// timerCallback()

//gives the current FFT block the next sample data
void MainComponent::pushNextSampleIntoFifo(float sample) noexcept {
    // if the fifo contains enough data, set a flag to say
    // that the next line should now be rendered..
    if (fifoIndex == fftSize) {
        if (!nextFFTBlockReady)
        {
            juce::zeromem(fftData, sizeof(fftData));
            memcpy(fftData, fifo, sizeof(fifo));
            nextFFTBlockReady = true;
        }
        fifoIndex = 0;
    }
    fifo[fifoIndex++] = sample;
}// pushNextSampleIntoFifo()

void MainComponent::drawNextLineOfSpectrogram() {
    const int imageHeight = spectrogramImage.getHeight();
    // do FFT
    fft.performFrequencyOnlyForwardTransform(fftData);
    //for each pixel position y in the spectrogram image
    for (int y = 1; y < imageHeight; ++y)
    {
        //skew the y proportion logarithmically and get use this value to index into the fftData array
        const float skewedProportionY = 1.0f - std::exp(std::log(y / (float)imageHeight) * 0.2f);
        const int fftDataIndex = juce::jlimit(0, fftSize / 2, (int)(skewedProportionY * fftSize / 2));
        maxValue = juce::jmax(std::log(fftData[fftDataIndex]), maxValue);
        //get the appropriate color for the magnitude at this position
        const float level = juce::jmap(std::log(fftData[fftDataIndex]), 0.0f, maxValue, 0.0f, 1.0f);
        spectrogramImage.setPixelAt(pixelX, y, juce::Colour::fromHSV(level, 1.0f, level, 1.0f));
        spectrogramImage.setPixelAt(pixelX + 1, y, juce::Colour::fromHSV(level, 1.0f, level, 1.0f));
    }
}// drawNextLineOfSpectrogram()

void MainComponent::openButtonClicked() {
    //Create the FileChooser object with a short message and allow the user to select only .wav files
    chooser = std::make_unique<juce::FileChooser>("Select a Wave file to play...", juce::File{}, "*.wav");
    // Pop up the FileChooser object
    auto chooserFlags = juce::FileBrowserComponent::openMode | juce::FileBrowserComponent::canSelectFiles;
    chooser->launchAsync(chooserFlags, [this](const juce::FileChooser& fc) {
        readInFileFFT(fc.getResult());
    });
}// openButtonClicked()

//reads in a file and dumps the sample data to the fileBuffer, then draws the spectrogram
void MainComponent::readInFileFFT(const juce::File& file) {
    // if () will succeed if the user actually selects a file(rather than cancelling)
    if (file != juce::File{}) {
        // attempt to create a reader for this particular file. 
        // This will return the nullptr value if it fails
        auto* reader = formatManager.createReaderFor(file); //reads in from the chosen file

        if (reader != nullptr) {
            // create a new AudioFormatReaderSource object using the reader we just created
            // We store the AudioFormatReaderSource object in a temporary std::unique_ptr 
            // object to avoid deleting a previously allocated AudioFormatReaderSource prematurely 
            // on subsequent commands to open a file.
            auto newSource = std::make_unique<juce::AudioFormatReaderSource>(reader, true);
            // The AudioFormatReaderSource object is connected to our AudioTransportSource object that is being used in our getNextAudioBlock()
            transportSource.setSource(newSource.get(), 0, nullptr, reader->sampleRate);
            const double duration = reader->lengthInSamples / reader->sampleRate;
            if (duration < 600) // limits input file to 600 seconds -> 10 mins
            {
                fileBuffer.setSize(reader->numChannels, reader->lengthInSamples);
                reader->read(&fileBuffer, 0, reader->lengthInSamples, 0, true, true);
                setAudioChannels(0, reader->numChannels);
                drawSpectrogram();
            }
            playButton.setEnabled(true);
            // we can safely store the AudioFormatReaderSource object in our readerSource member
            // we must transfer ownership from the local newSource variable by using std::unique_ptr::release()
            readerSource.reset(newSource.release());
        }
    }
}// readInFileFFT()

//draws the spectrogram using the data from the fileBuffer, which contains sample data from the read in file
void MainComponent::drawSpectrogram() {
    //clear the spectrogram
    position = 0;
    pixelX = 0;
    spectrogramImage.clear(spectrogramImage.getBounds(), juce::Colours::black);
    nextFFTBlockReady = false;
    juce::zeromem(fftData, sizeof(fftData));
    juce::zeromem(fifo, sizeof(fifo));

    //for each sample in the file buffer
    while (position < fileBuffer.getNumSamples()) {
        pushNextSampleIntoFifo(fileBuffer.getSample(0, position));
        if (nextFFTBlockReady) {
            drawNextLineOfSpectrogram();
            nextFFTBlockReady = false;
            pixelX += 2;
        }
        position++;
    }
    repaint();
}

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
}// changeListenerCallback()

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
}// changeState
#include "MainComponent.h"

//==============================================================================
MainComponent::MainComponent()
    :
    openButton("Select a File..."), 
    fft(fftOrder), 
    spectrogramImage(juce::Image::RGB, 660, 330, true), 
    constellationImage(juce::Image::RGB, 660, 330, true), 
    combinedImage(juce::Image::RGB, 1360, 330, true),
    currentStatus("Please Select a Video for Processing"),
    maxValue(1)
{
    // Buttons
    addAndMakeVisible(&openButton);
    openButton.onClick = [this] { openButtonClicked(); };

    setOpaque(true);

    setAudioChannels(1, 0);

    setSize (1400, 900);

    formatManager.registerBasicFormats(); // allows for .wav and .aaif files
}

MainComponent::~MainComponent() {
    // no audio play back
    shutdownAudio();
}

//==============================================================================
void MainComponent::prepareToPlay (int samplesPerBlockExpected, double sampleRate) {
    // no audio play back
    normalRange = makeRange::withCentre(float((double(fftSize) / sampleRate) * 20.f), float((double(fftSize) / sampleRate) * 20000.f), float((double(fftSize)/sampleRate)*1000.f));
}

void MainComponent::getNextAudioBlock (const juce::AudioSourceChannelInfo& bufferToFill) {
    // no audio play back
    bufferToFill.clearActiveBufferRegion();
}

void MainComponent::releaseResources() {
    transportSource.releaseResources();
}

//==============================================================================
void MainComponent::paint (juce::Graphics& g) {
    // background color
    g.fillAll (getLookAndFeel().findColour (juce::ResizableWindow::backgroundColourId));
    // three main images
    g.drawImageAt(spectrogramImage, 20, 20);
    g.drawImageAt(constellationImage, (getWidth() / 2)+20, 20);
    g.drawImageAt(combinedImage, 20, 380);
    // logo
    g.drawImageAt(logo, (getWidth() / 2)-160, 750);
    // title bars
    g.setColour(juce::Colour (27, 27, 27));
    g.fillRect(20, 20, 660, 30);
    g.fillRect((getWidth() / 2) + 20, 20, 660, 30);
    g.fillRect(20, 380, 1360, 30);
    // title bar text
    g.setFont(20.0f);
    g.setColour(juce::Colours::white);
    g.drawMultiLineText(currentStatus, (getWidth()-400), 800, 400);
    g.drawText(currentSizeAsString, getLocalBounds(), juce::Justification::bottomRight, true);
    g.drawMultiLineText("Spectrogram No Peaks", 270, 40, 400);
    g.drawMultiLineText("Peaks No Spectrogram", (getWidth() / 2) + 270, 40, 400);
    g.drawMultiLineText("Spectrogram With Peaks", 580, 400, 400);
}

void MainComponent::resized() {
    openButton.setBounds(100, 780, ((getWidth() - 20)/2)-320, 30);
    currentSizeAsString = juce::String(getWidth()) + " x " + juce::String(getHeight());
}


//gives the current FFT block the next sample data
void MainComponent::pushNextSampleIntoFifo(float sample) noexcept {
    // if the fifo contains enough data, set a flag to say
    // that the next line should now be rendered..
    if (fifoIndex == fftSize) {
        if (!nextFFTBlockReady)
        {
            std::fill(fftData.begin(), fftData.end(), 0.0f);
            std::copy(fifo.begin(), fifo.end(), fftData.begin());
            nextFFTBlockReady = true;
        }
        fifoIndex = 0;
    }
    fifo[(size_t)fifoIndex++] = sample;
}// pushNextSampleIntoFifo()


void MainComponent::drawNextLineOfSpectrogram() {
    auto imageHeight = spectrogramImage.getHeight();
    fft.performFrequencyOnlyForwardTransform(fftData.data());
    auto maxLevel = juce::FloatVectorOperations::findMinAndMax(fftData.data(), fftSize / 2);
    for (auto y = 1; y < imageHeight; ++y) {
        auto normalization = (float)y / imageHeight;
        auto fftDataIndex = normalRange.convertFrom0to1((1-normalization));
        auto level = juce::jmap(fftData[fftDataIndex], 0.0f, juce::jmax(maxLevel.getEnd(), 1e-5f), 0.0f, 1.0f);
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
    currentStatus = "Processing The Selected File...";
}// openButtonClicked()

void MainComponent::readInFileFFT(const juce::File& file) {
    if (file != juce::File{}) {
        auto* reader = formatManager.createReaderFor(file); //reads in from the chosen file
        if (reader != nullptr) {
            auto newSource = std::make_unique<juce::AudioFormatReaderSource>(reader, true);
            const double duration = reader->lengthInSamples / reader->sampleRate;
            if (duration < 600) {
                // limits input file to 600 seconds -> 10 mins
                fileBuffer.setSize(reader->numChannels, reader->lengthInSamples);
                reader->read(&fileBuffer, 0, reader->lengthInSamples, 0, true, true);
                position = 0;
                drawSpectrogram();
            }
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
    std::fill(fftData.begin(), fftData.end(), 0.0f);
    std::fill(fifo.begin(), fifo.end(), 0.0f);

    //for each sample in the file buffer
    while (position < fileBuffer.getNumSamples()) {
        pushNextSampleIntoFifo(fileBuffer.getSample(0, position));
        if (nextFFTBlockReady) {
            drawNextLineOfSpectrogram();
            nextFFTBlockReady = false;
            pixelX+=1;
        }
        position++;
    }
    repaint();
}// drawSpectrogram()

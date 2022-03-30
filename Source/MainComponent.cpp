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
    maxValue(1),
    normalRange(makeRange::withCentre(20.f, 20000.f, 10000.f))
    //sampleRate(44100) //sample rate (defaults to 44100, is updated later by reader)
{
    // Buttons
    addAndMakeVisible(&openButton);
    openButton.onClick = [this] { openButtonClicked(); };

    setOpaque(true);
    startTimerHz(60);

    setSize (1400, 900);

    formatManager.registerBasicFormats(); // allows for .wav and .aaif files
}

MainComponent::~MainComponent() {
    // no audio play back
}

//==============================================================================
void MainComponent::prepareToPlay (int samplesPerBlockExpected, double sampleRate) {
    // no audio play back
}

void MainComponent::getNextAudioBlock (const juce::AudioSourceChannelInfo& bufferToFill) {
    // no audio play back
    auto numInputChannels = fileBuffer.getNumChannels();
    auto numOutputChannels = bufferToFill.buffer->getNumChannels();

    auto outputSamplesRemaining = bufferToFill.numSamples;
    auto outputSamplesOffset = bufferToFill.startSample;

    while (outputSamplesRemaining > 0) {
        auto bufferSamplesRemaining = fileBuffer.getNumSamples() - position;
        auto samplesThisTime = juce::jmin(outputSamplesRemaining, bufferSamplesRemaining);

        for (auto channel = 0; channel < numOutputChannels; ++channel)
        {
            bufferToFill.buffer->copyFrom(channel,
                outputSamplesOffset,
                fileBuffer,
                channel % numInputChannels,
                position,
                samplesThisTime);
        }

        outputSamplesRemaining -= samplesThisTime;
        outputSamplesOffset += samplesThisTime;
        position += samplesThisTime;

        if (position == fileBuffer.getNumSamples())
            position = 0;
    }
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

//repaints at 60 hz when playing the wav file
void MainComponent::timerCallback() {
    if (nextFFTBlockReady)
    {
        drawNextLineOfSpectrogram();
        nextFFTBlockReady = false;
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
            std::fill(fftData.begin(), fftData.end(), 0.0f);
            std::copy(fifo.begin(), fifo.end(), fftData.begin());
            nextFFTBlockReady = true;
        }
        fifoIndex = 0;
    }
    fifo[(size_t)fifoIndex++] = sample;
}// pushNextSampleIntoFifo()

//void MainComponent::drawNextLineOfSpectrogram() {
//    auto rightHandEdge = spectrogramImage.getWidth() - 1;
//    auto imageHeight = spectrogramImage.getHeight();
//    spectrogramImage.moveImageSection(0, 0, 1, 0, rightHandEdge, imageHeight);
//    fft.performFrequencyOnlyForwardTransform(fftData.data());
//    auto maxLevel = juce::FloatVectorOperations::findMinAndMax(fftData.data(), fftSize / 2);
//    for (auto y = 1; y < imageHeight; ++y) {
//        auto skewedProportionY = 1.0f - std::exp(std::log((float)y / (float)imageHeight) * 2.0f);
//        auto fftDataIndex = (size_t)juce::jlimit(0, fftSize / 2, (int)(skewedProportionY * fftSize / 2));
//        auto level = juce::jmap(fftData[fftDataIndex], 0.0f, juce::jmax(maxLevel.getEnd(), 1e-5f), 0.0f, 1.0f);
//        spectrogramImage.setPixelAt(rightHandEdge, y, juce::Colour::fromHSV(level, 1.0f, level, 1.0f));
//    }
//
//}// drawNextLineOfSpectrogram()

//void MainComponent::drawNextLineOfSpectrogram() {
//    auto imageHeight = spectrogramImage.getHeight();
//    fft.performFrequencyOnlyForwardTransform(fftData.data());
//    for (auto y = 1; y < imageHeight; ++y) {
//        //skew the y proportion logarithmically and get use this value to index into the fftData array
//        const float skewedProportionY = 1.0f - std::exp(std::log(y / (float)imageHeight) * 0.2f);
//        const int fftDataIndex = juce::jlimit(0, fftSize / 2, (int)(skewedProportionY * fftSize / 2));
//        maxValue = juce::jmax(std::log(fftData[fftDataIndex]), maxValue);
//        //get the appropriate color for the magnitude at this position
//        const float level = juce::jmap(std::log(fftData[fftDataIndex]), 0.0f, maxValue, 0.0f, 1.0f);
//        spectrogramImage.setPixelAt(pixelX, y, juce::Colour::fromHSV(level, 1.0f, level, 1.0f));
//        spectrogramImage.setPixelAt(pixelX + 1, y, juce::Colour::fromHSV(level, 1.0f, level, 1.0f));
//    }
//
//}// drawNextLineOfSpectrogram()
 
//void MainComponent::feedNextLineOfSpectrogram(int x) {
//    auto imageHeight = spectrogramImage.getHeight();
//    fft.performFrequencyOnlyForwardTransform(fftData.data());
//    auto maxLevel = juce::FloatVectorOperations::findMinAndMax(fftData.data(), fftSize / 2);
//    for (auto y = 1; y < imageHeight; ++y) {
//        //auto skewedProportionY = 1.0f - std::exp(std::log((float)y / (float)imageHeight) * 2.0f);
//        auto normalization = (float)y / imageHeight;
//        //auto deNormalized = normalRange.convertFrom0to1(normalization);
//        //auto reNormalized = (deNormalized - 20.f) / 20000.f;
//        auto fftDataIndex =  (1 - normalization) * fftSize * .5f;
//
//        //auto fftDataIndex = (size_t)juce::jlimit(0, fftSize / 2, (int)(skewedProportionY * fftSize / 2));
//        auto level = juce::jmap(fftData[fftDataIndex], 0.0f, juce::jmax(maxLevel.getEnd(), 1e-5f), 0.0f, 1.0f);
//        // auto denormalValue = range.convertFrom0to1(value);
//        spectrogramImage.setPixelAt(x, y, juce::Colour::fromHSV(level, 1.0f, level, 1.0f));
//    }
//}

void MainComponent::drawNextLineOfSpectrogram() {
    auto rightHandEdge = spectrogramImage.getWidth() - 1;
    auto imageHeight = spectrogramImage.getHeight();
    auto imageWidth = spectrogramImage.getWidth();
    spectrogramImage.moveImageSection(0, 0, 1, 0, rightHandEdge, imageHeight);
    fft.performFrequencyOnlyForwardTransform(fftData.data());
    auto maxLevel = juce::FloatVectorOperations::findMinAndMax(fftData.data(), fftSize / 2);
    for (auto y = 1; y < imageHeight; ++y) {
        //auto skewedProportionY = 1.0f - std::exp(std::log((float)y / (float)imageHeight) * 2.0f);
        auto normalization = (float)y / imageHeight;
        //auto deNormalized = normalRange.convertFrom0to1(normalization);
        //auto reNormalized = (deNormalized - 20.f) / 20000.f;
        auto fftDataIndex =  (1 - normalization) * fftSize * .5f;

        //auto fftDataIndex = (size_t)juce::jlimit(0, fftSize / 2, (int)(skewedProportionY * fftSize / 2));
        auto level = juce::jmap(fftData[fftDataIndex], 0.0f, juce::jmax(maxLevel.getEnd(), 1e-5f), 0.0f, 1.0f);
        // auto denormalValue = range.convertFrom0to1(value);
        spectrogramImage.setPixelAt(rightHandEdge, y, juce::Colour::fromHSV(level, 1.0f, level, 1.0f));
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
                //setAudioChannels(0, reader->numChannels);
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
            pixelX+=2;
        }
        position++;
    }
    repaint();
}// drawSpectrogram()

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
    // set range for data normalization
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
    // do the fft
    fft.performFrequencyOnlyForwardTransform(fftData.data());
    auto maxLevel = juce::FloatVectorOperations::findMinAndMax(fftData.data(), fftSize / 2);
    // prepare vectors for push_back()
    constellationData.resize(pixelX+1);
    hashingData.resize(pixelX + 1);
    // for each pixel on the y-axis
    for (auto y = 1; y < imageHeight; ++y) {
        // normalize the data
        auto normalization = (float)y / imageHeight;
        auto fftDataIndex = normalRange.convertFrom0to1((1-normalization));
        auto level = juce::jmap(fftData[fftDataIndex], 0.0f, juce::jmax(maxLevel.getEnd(), 1e-5f), 0.0f, 1.0f);
        // store key points
        hashingData[pixelX].push_back(std::make_pair(fftData[fftDataIndex], y)); // implicit caste to int for fftData[fftDataIndex]
        constellationData[pixelX].push_back(std::make_pair(level,y));
        // draw the image
        spectrogramImage.setPixelAt(pixelX, y, juce::Colour::fromHSV(level, 1.0f, level, 1.0f));
        combinedImage.setPixelAt(pixelX, y, juce::Colour::fromHSV(level, 1.0f, level, 1.0f));
    }
}// drawNextLineOfSpectrogram()

void MainComponent::drawConstellationImage() {
    for (int x = 0; x < (int)constellationData.size(); x++) {
        generatePeakPoints(x);
        // draw these points on the image
        for (int y = 0; y < (int)peakPoints.size(); y++) {
            //DBG(y << " PixelX: " << x << " PixelY: " << peakPoints[y].second << " FrequencyLevel: " << peakPoints[y].first);
            constellationImage.setPixelAt(x, peakPoints[y].second, juce::Colours::white);
            combinedImage.setPixelAt(x, peakPoints[y].second, juce::Colours::white);
        }
    }
}// end constellationImage()

void MainComponent::generatePeakPoints(int x) {
    // sort to get the most prominent points
    std::sort(constellationData[x].begin(), constellationData[x].end());
    // take the top 5-10 'strongest'/'robust' points
    std::vector<std::pair<float, int>>::const_iterator first = constellationData[x].end() - 5;
    std::vector<std::pair<float, int>>::const_iterator last = constellationData[x].end();
    peakPoints.assign(first, last);
}// end generatePeakPoints(

void MainComponent::generateFingerprint() {
    for (int x = 0; x < (int)hashingData.size(); x++) {
        // sort to get the most prominent points (peak frequency points)
        std::sort(hashingData[x].begin(), hashingData[x].end());

        // take the top 5-10 'strongest'/'robust' points
        std::vector<std::pair<int, int>>::const_iterator first = hashingData[x].end() - 5;
        std::vector<std::pair<int, int>>::const_iterator last = hashingData[x].end();
        std::vector<std::pair<int, int>> peakPoints(first, last);

        // sort to get in order of increasing y value ... 0->1->2->3 which is from .second() of the pair
        std::sort(peakPoints.begin(), peakPoints.end(),
        [](const std::pair<int, int>& x, const std::pair<int, int>& y)
        {
            return x.second < y.second;
        });

        // hash these peak frequency values in their correctn increasing order
        // "hash a bin"
        std::hash<int> hasher;
        long fingerprint;
        for (int y = 0; y < (int)peakPoints.size(); y++) {
            DBG(y << " PixelX: " << x << " PixelY: " << peakPoints[y].second << " Frequency: " << peakPoints[y].first);
            fingerprint += hasher(peakPoints[y].second);
        }
        //DBG("fingerprint for pixelX = " << x << " is " << fingerprint);
        fingerprint = 0;
    }
}// end generateFingerprint()

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
    constellationImage.clear(constellationImage.getBounds(), juce::Colours::black);
    combinedImage.clear(combinedImage.getBounds(), juce::Colours::black);
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
    drawConstellationImage();
    generateFingerprint();
    repaint();
}// drawSpectrogram()

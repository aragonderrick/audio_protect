#include "MainComponent.h"
#include <sstream>
#include <algorithm>

//==============================================================================
MainComponent::MainComponent()
    :
    openButton("Fingerprint a New File"),
    checkButton("Audio Protect an Existing File"),
    fft(fftOrder),
    spectrogramImage(juce::Image::RGB, 660, 330, true),
    constellationImage(juce::Image::RGB, 660, 330, true),
    combinedImage(juce::Image::RGB, 1360, 330, true),
    maxValue(1)
{
    // Buttons
    addAndMakeVisible(&openButton);
    openButton.onClick = [this] { openButtonClicked(); };
    addAndMakeVisible(&checkButton);
    checkButton.onClick = [this] { checkButtonClicked(); };

    setOpaque(true);

    setAudioChannels(1, 0);

    setSize(1400, 900);

    formatManager.registerBasicFormats(); // allows for .wav and .aaif files

    populateFingerprints();
}

MainComponent::~MainComponent() {
    // no audio play back
    shutdownAudio();
}

//==============================================================================
void MainComponent::prepareToPlay(int samplesPerBlockExpected, double sampleRate) {
    // no audio play back
    // set range for data normalization
    normalRange = makeRange::withCentre(float((double(fftSize) / sampleRate) * 20.f), float((double(fftSize) / sampleRate) * 20000.f), float((double(fftSize) / sampleRate) * 1000.f));
}

void MainComponent::getNextAudioBlock(const juce::AudioSourceChannelInfo& bufferToFill) {
    // no audio play back
    bufferToFill.clearActiveBufferRegion();
}

void MainComponent::releaseResources() {
    transportSource.releaseResources();
}

//==============================================================================
void MainComponent::paint(juce::Graphics& g) {
    // background color
    g.fillAll(getLookAndFeel().findColour(juce::ResizableWindow::backgroundColourId));
    // three main images
    g.drawImageAt(spectrogramImage, 20, 20);
    g.drawImageAt(constellationImage, (getWidth() / 2) + 20, 20);
    g.drawImageAt(combinedImage, 20, 380);
    // logo
    g.drawImageAt(logo, (getWidth() / 2) - 160, 750);
    // title bars
    g.setColour(juce::Colour(27, 27, 27));
    g.fillRect(20, 20, 660, 30);
    g.fillRect((getWidth() / 2) + 20, 20, 660, 30);
    g.fillRect(20, 380, 1360, 30);
    // title bar text
    g.setFont(20.0f);
    g.setColour(juce::Colours::white);
    g.drawMultiLineText(currentStatus, (getWidth() - 440), 800, 400);
    //g.drawText(currentSizeAsString, getLocalBounds(), juce::Justification::bottomRight, true);
    g.drawMultiLineText("Spectrogram No Peaks", 270, 40, 400);
    g.drawMultiLineText("Peaks No Spectrogram", (getWidth() / 2) + 270, 40, 400);
    g.drawMultiLineText("Spectrogram With Peaks", 580, 400, 400);
}

void MainComponent::resized() {
    openButton.setBounds(100, 760, ((getWidth() - 20) / 2) - 320, 30);
    checkButton.setBounds(100, 800, ((getWidth() - 20) / 2) - 320, 30);
    //currentSizeAsString = juce::String(getWidth()) + " x " + juce::String(getHeight());
}// end resize()

void MainComponent::populateFingerprints() {
    // UNCOMMENT THIS PORTION OF THE CODE TO POPULATE A TEXT FILE WITH FFT DATA (THE DATABASE)
    //*******************************************************************************
    // 
    //draw = false;
    //juce::File audioFiles("C:/Users/arago/OneDrive/Desktop/Spring2022/CSCI490/TestFilesWav");
    //juce::Array<juce::File> wavFiles;
    //wavFiles = audioFiles.findChildFiles(2, false, "*.wav");
    //for (int i = 0; i < (int)wavFiles.size(); i++) {
    //    readInFileFFT(wavFiles[i]);
    //}
    //draw = true;
    //hashtable.printAll();
    // 
    //*******************************************************************************
    const juce::File fingerprintData("C:/Users/arago/OneDrive/Desktop/Spring2022/CSCI490/formated_database.txt");
    if (!fingerprintData.existsAsFile()) {
        DBG(fingerprintData.getFileName() << " doesnt not exist");
        return;  // file doesn't exist
    }
    juce::FileInputStream inputStream(fingerprintData);
    if (!inputStream.openedOk()) {
        DBG("failed to open " << inputStream.getFile().getFileName());
        return;  // failed to open
    }
    bool hasFingerprint = false;
    while (!inputStream.isExhausted()) {
        std::istringstream ss(inputStream.readNextLine().toStdString());
        std::string word;
        long fp;
        ss >> word;
        if (word == "*") {
            hasFingerprint = true;
        }
        else if (hasFingerprint) {
            fp = std::stol(word);
            ss = (std::istringstream)inputStream.readNextLine().toStdString();
            ss >> word;
            int num_prints = std::stoi(word);
            //DBG(word);
            for (int i = 0; i < num_prints; i++) {
                ss = (std::istringstream)inputStream.readNextLine().toStdString();
                ss >> word;
                //DBG(word);
                std::string name = word;
                ss >> word;
                int seconds = std::stoi(word);
                hashtable.insertElement(fp, seconds, name);
            }
            hasFingerprint = false;
        }
    }
    currentStatus = "Populated the Database with 25 Songs.";
}// end populateFingerprints()

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
    constellationData.resize(pixelX + 1);
    hashingData.resize(pixelX + 1);
    // for each pixel on the y-axis
    for (auto y = 1; y < imageHeight; ++y) {
        // normalize the data
        auto normalization = (float)y / imageHeight;
        auto fftDataIndex = normalRange.convertFrom0to1((1 - normalization));
        auto level = juce::jmap(fftData[fftDataIndex], 0.0f, juce::jmax(maxLevel.getEnd(), 1e-5f), 0.0f, 1.0f);
        // store key points
        hashingData[pixelX].push_back(std::make_pair(fftData[fftDataIndex], y)); // implicit caste to int for fftData[fftDataIndex]
        constellationData[pixelX].push_back(std::make_pair(level, y));
        if (draw) {
            // draw the image
            spectrogramImage.setPixelAt(pixelX, y, juce::Colour::fromHSV(level, 1.0f, level, 1.0f));
            combinedImage.setPixelAt(pixelX, y, juce::Colour::fromHSV(level, 1.0f, level, 1.0f));
        }
    }
}// drawNextLineOfSpectrogram()

void MainComponent::drawConstellationImage() {
    for (int x = 0; x < (int)constellationData.size(); x++) {
        // sort to get the most prominent points
        std::sort(constellationData[x].begin(), constellationData[x].end());
        // take the top 5-10 'strongest'/'robust' points
        std::vector<std::pair<float, int>>::const_iterator first = constellationData[x].end() - 5;
        std::vector<std::pair<float, int>>::const_iterator last = constellationData[x].end();
        std::vector<std::pair<float, int>> peakPoints(first, last);
        // draw these points on the image
        if (draw) {
            for (int y = 0; y < (int)peakPoints.size(); y++) {
                //DBG(y << " PixelX: " << x << " PixelY: " << peakPoints[y].second << " FrequencyLevel: " << peakPoints[y].first);
                constellationImage.setPixelAt(x, peakPoints[y].second, juce::Colours::white);
                combinedImage.setPixelAt(x, peakPoints[y].second, juce::Colours::white);
            }
        }
    }
}// end constellationImage()

void MainComponent::generateFingerprint() {
    int frames_per_second = std::floor(hashingData.size() / duration);
    int seconds_passed = 0;
    for (int x = 0; x < (int)hashingData.size(); x += frames_per_second) {
        // Every 1 second in the data, I will only fingerprint these points
        // sort to get the most prominent points (peak frequency points)
        std::sort(hashingData[x].begin(), hashingData[x].end());
        std::vector<std::pair<int, int>> peakPoints;
        int i = 1;
        // GRAB UNIQUE VALUES from sorted hashingdata[], take the top 5 'strongest'/'robust' points
        while (peakPoints.size() < 5 && (hashingData[x].end() - i) != hashingData[x].begin()) {
            int searchVal = (hashingData[x].end() - i)->first;
            //DBG("Is " << searchVal << " in the vector?");
            if (std::find_if(peakPoints.begin(), peakPoints.end(), [searchVal](const auto& pair) { return pair.first == searchVal; }) == peakPoints.end()) {
                // insert if it isnt already a peak point
                peakPoints.push_back(std::make_pair((hashingData[x].end() - i)->first, (hashingData[x].end() - i)->second));
            }
            i++;
        }

        // sort to get in order of increasing y value ... 0->1->2->3 which is from .second() of the pair
        std::sort(peakPoints.begin(), peakPoints.end(),
            [](const std::pair<int, int>& x, const std::pair<int, int>& y)
        {
            return x.second < y.second;
        });

        // hash these peak frequency values in their correct increasing order "hash a bin"
        std::hash<int> hasher;
        long fingerprint = 0;
        for (int y = 0; y < (int)peakPoints.size(); y++) {
            //DBG("Seconds Passed: " << seconds_passed << " => " << y << " PixelX: " << x << " PixelY: " << peakPoints[y].second << " Frequency: " << peakPoints[y].first);
            fingerprint += hasher(peakPoints[y].second);
        }
        //DBG("fingerprint for pixelX = " << x << " is " << fingerprint);

        if (draw) {
            hashtable.insertElement(fingerprint, seconds_passed, song_name);
        }
        else {
            std::vector<std::pair<std::string, int>> song_matches;
            if (hashtable.check(fingerprint, seconds_passed, song_matches)) {
                potential_matches.push_back(song_matches);
            }
        }

        fingerprint = 0;
        seconds_passed++;
    }
    //hashtable.printAll();
}// end generateFingerprint()

void MainComponent::makePrediction() {
    // prepare map for sorting and indexing predictions
    std::map<int, std::vector<std::string>> matches;
    //sort through potential matches and bin them by their offset value
    for (int i = 0; i < potential_matches.size(); i++) {
        for (int j = 0; j < potential_matches[i].size(); j++) {
            matches[potential_matches[i][j].second].push_back(potential_matches[i][j].first);
        }
    }

    std::string guestimate = "";
    int guestimate_occurance = 1;
    // loop through all of the offsets
    for (auto it = matches.begin(); it != matches.end(); it++) {
        // if a bin contains enough songs to potentially changhe our guess, look at songs in the bin
        if (it->second.size() >= guestimate_occurance) {
            // will strack accurance of song and its name with map
            std::map<string, int> occurence;
            auto it2 = it;
            // for this offset AND the next 3 offsets (due to JUCE bad timing functions)
            for (int i = 0; i < 4 && it2 != matches.end(); it2++, i++) {
                // track how aften a song occures for that given offset
                for (auto dp : it2->second) {
                    occurence[dp]++;
                }
            }
            // look for the most common occurance and this will be our current best guess
            for (auto it3 : occurence) {
                if (it3.second >= guestimate_occurance) {
                    guestimate_occurance = it3.second;
                    guestimate = it3.first;
                }
            }
        }
    }
    currentStatus = "Detected " + guestimate;
    //DBG(guestimate);
}// end makePrediction()

void MainComponent::openButtonClicked() {
    draw = true;
    //Create the FileChooser object with a short message and allow the user to select only .wav files
    chooser = std::make_unique<juce::FileChooser>("Select a Wave file to play...", juce::File{}, "*.wav");
    // Pop up the FileChooser object
    auto chooserFlags = juce::FileBrowserComponent::openMode | juce::FileBrowserComponent::canSelectFiles;
    chooser->launchAsync(chooserFlags, [this](const juce::FileChooser& fc) {
        readInFileFFT(fc.getResult());
    });
}// openButtonClicked()

void MainComponent::checkButtonClicked() {
    draw = false;
    //Create the FileChooser object with a short message and allow the user to select only .wav files
    chooser = std::make_unique<juce::FileChooser>("Select a Wave file to check...", juce::File{}, "*.wav");
    // Pop up the FileChooser object
    auto chooserFlags = juce::FileBrowserComponent::openMode | juce::FileBrowserComponent::canSelectFiles;
    chooser->launchAsync(chooserFlags, [this](const juce::FileChooser& fc) {
        readInFileFFT(fc.getResult());
    });
}// openButtonClicked()

void MainComponent::readInFileFFT(const juce::File& file) {
    currentStatus = "Fingerprinted " + file.getFileName().toStdString();
    song_name = file.getFileName().toStdString();
    if (file != juce::File{}) {
        auto* reader = formatManager.createReaderFor(file); //reads in from the chosen file
        if (reader != nullptr) {
            auto newSource = std::make_unique<juce::AudioFormatReaderSource>(reader, true);
            duration = reader->lengthInSamples / reader->sampleRate;
            fileBuffer.clear();
            if (duration < 600) {
                // limits input file to 600 seconds -> 10 mins
                fileBuffer.setSize(reader->numChannels, reader->lengthInSamples);
                reader->read(&fileBuffer, 0, reader->lengthInSamples, 0, true, true);
                position = 0;
                drawSpectrogram();
            }
        }
    }
}// readInFileFFT()

//draws the spectrogram using the data from the fileBuffer, which contains sample data from the read in file
void MainComponent::drawSpectrogram() {
    // reset fft variables
    position = 0;
    pixelX = 0;
    nextFFTBlockReady = false;
    std::fill(fftData.begin(), fftData.end(), 0.0f);
    std::fill(fifo.begin(), fifo.end(), 0.0f);
    fifoIndex = 0;
    std::vector<std::vector<std::pair<std::string, int>>> new_one;
    potential_matches = new_one;

    //clear the images
    spectrogramImage.clear(spectrogramImage.getBounds(), juce::Colours::black);
    constellationImage.clear(constellationImage.getBounds(), juce::Colours::black);
    combinedImage.clear(combinedImage.getBounds(), juce::Colours::black);

    //for each sample in the file buffer
    while (position < fileBuffer.getNumSamples()) {
        pushNextSampleIntoFifo(fileBuffer.getSample(0, position));
        if (nextFFTBlockReady) {
            drawNextLineOfSpectrogram();
            nextFFTBlockReady = false;
            pixelX += 1;
        }
        position++;
    }
    // draw the images 
    drawConstellationImage();
    generateFingerprint();
    // make predictions
    if (potential_matches.size() && !draw) {
        makePrediction();
    }
    repaint();
}// drawSpectrogram()

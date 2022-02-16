# audio_protect
An audio fingerprinting desktop application designed to detect and protect content creators against copyright strikes.

Audio Protect is a desktop application built with the open-source cross-platform framework, JUCE (https://juce.com/).

The application can be launched on any operating system and allows the user/content creator to upload a video source.

The video's audio will be seperated and processed through a FFT (Fast Fourier Transform https://en.wikipedia.org/wiki/Fast_Fourier_transform) and plotted two-deminsionally with the X-axis representing time, the Y-axis reprsenting the frequency (Hertz). 

This graph is called a spectrogram (https://en.wikipedia.org/wiki/Spectrogram). The spectrogram will then be processed by each 100ms frame/bin and track the 10 highest frequency peaks. 

The main chunk of the algorithm lies within the hashing of these points. The key is to increase the lookup time while decreasing the storage space required as well as reduce potential clashes in the hash-table.

The inspiration for my implimentation of this fingerprinting algorihm came from Shazam. Please feel free to review my source code, as I feel having open-source code leads to the highest level of transparency as well as contantly looking to improve the application.

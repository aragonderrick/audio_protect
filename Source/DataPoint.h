/*
  ==============================================================================

    DataPoint.h
    Created: 21 Apr 2022 9:26:38pm
    Author:  arago

  ==============================================================================
*/
#include <JuceHeader.h>
#include <string>
#pragma once
class DataPoint {

public:
    DataPoint(std::string sd, int t);
    int getTime();
    std::string getSongId();

private:
    int time;
    std::string songId;
};

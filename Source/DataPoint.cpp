/*
  ==============================================================================

    DataPoint.cpp
    Created: 21 Apr 2022 9:26:38pm
    Author:  arago

  ==============================================================================
*/

#include "DataPoint.h"

DataPoint::DataPoint(std::string sd, int t) {
    songId = sd;
    time = t;
}// end DataPoint()

int DataPoint::getTime() {
    return time;
}// end getTime()

std::string DataPoint::getSongId() {
    return songId;
}// end getSongId()

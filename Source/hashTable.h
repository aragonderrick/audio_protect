/*
  ==============================================================================

    hashTable.h
    Created: 11 Apr 2022 11:51:31pm
    Author:  arago

    The following code was changed and adapted from the follwoing tutorial:
    https://www.educative.io/edpresso/how-to-implement-a-hash-table-in-cpp

  ==============================================================================
*/

#pragma once
#include <JuceHeader.h>
#include "DataPoint.h"
#include <iostream>
#include <list>
#include <map>
#include <vector>
#include <string>

using namespace std;

class HashTable {
public:
    // Constructor to create a hash table with 'n' indices:

    // Insert data in the hash table:
    void insertElement(long fp, int time, std::string name);

    // check for potential matches
    bool check(long fingerprint, int time, std::vector<std::pair<std::string, int>> &matches);

    // print all values in map
    void printAll();

private:
    std::map<long, std::vector<DataPoint>> table;
};

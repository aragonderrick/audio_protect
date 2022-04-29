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
    HashTable(int n);

    // Insert data in the hash table:
    void insertElement(long fp, int time, std::string name);

    void printAll();

private:
    //list<long>* table;
    std::map<long, std::vector<DataPoint>> table;

};

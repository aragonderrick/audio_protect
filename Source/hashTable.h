/*
  ==============================================================================

    hashTable.cpp
    Created: 11 Apr 2022 11:51:31pm
    Author:  arago

  ==============================================================================
*/

#include "hashTable.h"
// Constructor to create a hash table with 'n' indices:
HashTable::HashTable(int n) {

    // Do nothing for now

}// end HashTable()

// Insert data in the hash table:
void HashTable::insertElement(long fp, int time, std::string name) {
    // insert the song name and time into the table/map
    // check if fingerprint exists
    //if (table[fp].size()) {
    //    // make sure its not the same file
    //    for (int i = 0; i < (int)table[fp].size(); i++) {
    //        if (name == table[fp][i].getSongId()) {
    //            // if it is then add it 
    //            table[fp].push_back(DataPoint(name, time));
    //        }
    //        else {
    //            DBG("\n==== WARNING SONG EXISTS ====\n");
    //            table[fp].push_back(DataPoint(name, time));
    //        }
    //    }
    //}
    //else {
    //    table[fp].push_back(DataPoint(name, time));
    //}
     table[fp].push_back(DataPoint(name, time));

}// end insertElement()

void HashTable::printAll() {
    DBG("NUMBER OF FINGER PRINTS: " << table.size());
    for (auto const& [key, val] : table) {
        //DBG("*\n" << key);
        //DBG(val.size());
        DBG("\nFingerprint: " << key);
        DBG("Songs with this finger print: ");
        for (auto dp : val) {
            DBG(dp.getSongId() << " " << dp.getTime() << ",");
            //DBG(dp.getSongId() << " " << dp.getTime());
        }
    }
}// end printAll()

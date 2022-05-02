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
    table[fp].push_back(DataPoint(name, time));

}// end insertElement()

bool HashTable::check(long fingerprint, int time, std::vector<std::pair<std::string, int>> &matches) {
    if (table[fingerprint].size()) {
        // fingerprint exists
        for (auto it = table[fingerprint].begin(); it != table[fingerprint].end(); it++) {
            matches.push_back(std::make_pair(it->getSongId(), (it->getTime() - time)));
        }
        return true;
    }
    return false;
}

void HashTable::printAll() {
    DBG("NUMBER OF FINGER PRINTS: " << table.size());
    for (auto const& [key, val] : table) {
        DBG("*\n" << key);
        DBG(val.size());
        //DBG("\nFingerprint: " << key);
        //DBG("Songs with this finger print: ");
        for (auto dp : val) {
            //DBG(dp.getSongId() << " " << dp.getTime() << ",");
            DBG(dp.getSongId() << " " << dp.getTime());
        }
    }
}// end printAll()

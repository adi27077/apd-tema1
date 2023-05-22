#ifndef UTILS_H
#define UTILS_H

#include <string>
#include <vector>
#include <queue>
#include <unordered_map>
#include <unordered_set>

using namespace std;

typedef struct MapperArgs{
    const int threadID;
    queue<string> *fileNames;
    vector<unordered_map<int, unordered_set<int> *> *> *mapperOutput;
    unordered_map<int, unordered_set<int> *> *powers;

    MapperArgs(int threadID, queue<string> *fileNames, vector<unordered_map<int, unordered_set<int> *> *> *mapperOutput,
                unordered_map<int, unordered_set<int> *> *powers)
        : threadID(threadID), fileNames(fileNames), mapperOutput(mapperOutput), powers(powers) {}
    
}MapperArgs;

typedef struct ReducerArgs{
    const int threadID;
    vector<unordered_map<int, unordered_set<int> *> *> *mapperOutput;

    ReducerArgs(int threadID, vector<unordered_map<int, unordered_set<int> *> *> *mapperOutput)
        : threadID(threadID), mapperOutput(mapperOutput) {}
}ReducerArgs;

#endif

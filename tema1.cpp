#include <iostream>
#include <fstream>
#include <cmath>
#include <pthread.h>

#include "utils.h"

pthread_mutex_t mutex;
pthread_barrier_t barrier;

void computePowers(unordered_map<int, unordered_set<int> *> *powers, const int nReducers) {
    // Compute powers for each exponent corresponding to a reducer (exponent = reducerId + 2)
    // and store them in sets. Add the sets to the map, where the key is the exponent.
    for (int i = 2; i < nReducers + 2; i++) {
        auto *v = new unordered_set<int>();
        for (int j = 1; pow(j, i) <= INT32_MAX; j++) {
            v->insert(pow(j, i));
        }

        powers->insert({i, v});
    }
}

void *mapper(void *arg) {
    // Get the mapper arguments from the structure
    const auto *args = static_cast<MapperArgs *>(arg);
    auto *fileNames = args->fileNames;
    auto *mapperOutput = args->mapperOutput;
    auto *powers = args->powers;
    const int threadID = args->threadID;

    // Declare a map which is used to store the results of the current mapper
    auto *map = new unordered_map<int, unordered_set<int> *>();
    
    // While there are files in the queue, we open them
    while (!fileNames->empty()) {
        // Use a mutex to prevent race conditions while accessing and modifying the queue
        pthread_mutex_lock(&mutex);
        if (fileNames->empty()) { 
            pthread_mutex_unlock(&mutex);
            break;
        }
        string fileName = fileNames->front();
        fileNames->pop();
        pthread_mutex_unlock(&mutex);

        ifstream fin(fileName);

        int lines;
        
        fin >> lines;
        for (int i = 0; i < lines; i++) {
            int n;
            fin >> n;

            // For each number read from the file, we check if it's a power of any number with
            // the exponents corresponding to the reducers
            for (const auto it : *powers) {
                const auto exp = it.first;
                const auto *s = it.second;

                // Check if the numbers read is in the powers table
                if (s->find(n) != s->end()) {
                    
                    // If it is, we add it to the set of the map corresponding to the exponent
                    // We use an unordered set to make sure each number appears only once
                    if (map->find(exp) == map->end()) {
                        auto *aux = new unordered_set<int>();
                        aux->insert(n);
                        map->insert({exp, aux});
                    } else {
                        map->at(exp)->insert(n);
                    }  
                }
            }

        }

        fin.close();

    }

    // After the mapper thread finishes all files, we add the map to the vector
    // containing all maps and we use a mutex to prevent race conditions
    pthread_mutex_lock(&mutex);
    mapperOutput->insert(mapperOutput->begin() + threadID, map);
    pthread_mutex_unlock(&mutex);
    
    // We place a barrier here to make sure all the mappers finished before the reducers start
    pthread_barrier_wait(&barrier);

    delete args;

    pthread_exit(NULL);
}

void *reducer(void *arg) {
    // Same purpose as the barrier at the end of the mapper function
    pthread_barrier_wait(&barrier);
    
    const auto *args = static_cast<ReducerArgs *>(arg);
    const auto *mapperOutput = args->mapperOutput;
    const int threadID = args->threadID;

    ofstream fout("out" + to_string(threadID + 2) + ".txt");

    auto *reducedSet = new unordered_set<int>();

    // We iterate through every map created by the mappers
    for (int i = 0; i < static_cast<int>(mapperOutput->size()); i++) {
        // Each reducer considers only the entries in the map corresponding to its exponent
        if (mapperOutput->at(i) != nullptr && mapperOutput->at(i)->find(threadID + 2) != mapperOutput->at(i)->end()) {
            // We extract the corresponding set if it exists in the map
            const auto *aux = mapperOutput->at(i)->at(threadID + 2);
            for (auto it = aux->begin(); it != aux->end(); it++) {
                // Add the numbers to the final set which is the result of the reducer
                reducedSet->insert(*it);
            }
        }  
    }

    // Print the number of unique occurences of the numbers which have been found as
    // perfect powers of the current exponent, which is the size of the resulting set
    fout << reducedSet->size();

    fout.close();

    delete reducedSet;
    delete args;

    pthread_exit(NULL);
}

int main(int argc, char *argv[]) {
    ifstream fin(argv[3]);

    const int nMappers = atoi(argv[1]), nReducers = atoi(argv[2]);

    auto *powers = new unordered_map<int, unordered_set<int> *>(nMappers);
    computePowers(powers, nReducers);

    auto *fileNames = new queue<string>();
    auto *mapperOutput = new vector<unordered_map<int, unordered_set<int> *> *>(nMappers);
    
    // Read file names and place them in the queue
    int nFiles;
    fin >> nFiles;
    for (int i = 0; i < nFiles; i++) {
        string fileName;
        fin >> fileName;
        fileNames->emplace(fileName);
    }

    pthread_t tid[nMappers + nReducers];

    pthread_mutex_init(&mutex, NULL);
    pthread_barrier_init(&barrier, NULL, nMappers + nReducers);

    // Start threads for mappers and reducers
    for (int i = 0; i < nMappers + nReducers; i++) {
        if (i < nMappers) {
            auto *args = new MapperArgs(i, fileNames, mapperOutput, powers);
            pthread_create(&tid[i], NULL, mapper, static_cast<void *>(args));
        } else {
            auto *args = new ReducerArgs(i - nMappers, mapperOutput);
            pthread_create(&tid[i], NULL, reducer, static_cast<void *>(args));
        }

    }

    for (int i = 0; i < nMappers + nReducers; i++) {
        pthread_join(tid[i], NULL);
    }

    pthread_mutex_destroy(&mutex);
    pthread_barrier_destroy(&barrier);

    delete fileNames;
    delete mapperOutput;
    delete powers;

    fin.close();

    return 0;
}

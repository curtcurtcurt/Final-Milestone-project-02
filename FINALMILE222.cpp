#include <iostream>
#include <fstream>
#include <sstream>
#include <unordered_map>
#include <list>
#include <queue>
#include <vector>
#include <algorithm>
#include <random>
#include <ctime>

using namespace std;

class ICacheStrategy {
public:
    virtual bool get(const string& key, double& population) = 0;
    virtual void put(const string& key, double population) = 0;
    virtual void printCache() const = 0;
    virtual ~ICacheStrategy() {}
};








class LFUCache : public ICacheStrategy {
    struct CacheEntry {
        string key;
        double population;
        int freq;
        int time;
    };

    int capacity, currentTime;
    unordered_map<string, CacheEntry> cacheMap;




public:
    LFUCache(int cap) : capacity(cap), currentTime(0) {}

    bool get(const string& key, double& population) override {
        auto it = cacheMap.find(key);
        if (it == cacheMap.end()) return false;
        it->second.freq++;
        it->second.time = ++currentTime;
        population = it->second.population;
        return true;
    }

    void put(const string& key, double population) override {
        currentTime++;
        if (cacheMap.count(key)) {
            cacheMap[key].population = population;
            cacheMap[key].freq++;
            cacheMap[key].time = currentTime;
            return;
        }

        if (cacheMap.size() == capacity) {
            string evictKey;
            int minFreq = INT_MAX, minTime = INT_MAX;
            for (const auto& [k, entry] : cacheMap) {
                if (entry.freq< minFreq || (entry.freq == minFreq && entry.time < minTime)) {
                    minFreq= entry.freq;
                    minTime= entry.time;
                    evictKey = k;
                }
            }
            cacheMap.erase(evictKey);
        }

        cacheMap[key] = {key, population, 1, currentTime};
    }

    void printCache() const override {
        cout << "LFU Cache:\n";
        for (const auto& [key, entry] : cacheMap) {
            cout << key << " => " << entry.population << " (freq: " << entry.freq << ")\n";
        }
    }
};

class FIFOCache : public ICacheStrategy {
    int capacity;
    queue<string> order;
    unordered_map<string, double> cacheMap;

public:
    FIFOCache(int cap) : capacity(cap) {}

    bool get(const string& key, double& population) override {
        auto it = cacheMap.find(key);
        if (it == cacheMap.end()) return false;
        population = it->second;
        return true;
    }

    void put(const string& key, double population) override {
        if (cacheMap.count(key)) return;

        if (cacheMap.size() == capacity) {
            string oldest = order.front(); order.pop();
            cacheMap.erase(oldest);
        }

        cacheMap[key] = population;
        order.push(key);
    }

    void printCache() const override {
        cout << "FIFO Cache:\n";
        for (const auto& [key, val] : cacheMap)
            cout << key << " => " << val << endl;
    }
};




class RandomCache : public ICacheStrategy {
    int capacity;
    unordered_map<string, double> cacheMap;
    vector<string> keys;
    mt19937 rng;

public:
    RandomCache(int cap) : capacity(cap), rng(time(0)) {}

    bool get(const string& key, double& population) override {
        auto it = cacheMap.find(key);
        if (it == cacheMap.end()) return false;
        population = it->second;
        return true;
    }

    void put(const string& key, double population) override {
        if (cacheMap.count(key)) return;

        if (cacheMap.size() == capacity) {
            uniform_int_distribution<int> dist(0, keys.size() - 1);
            int idx = dist(rng);
            cacheMap.erase(keys[idx]);
            keys.erase(keys.begin() + idx);
        }

        cacheMap[key] = population;
        keys.push_back(key);
    }

    void printCache() const override {
        cout << "Random Cache:\n";
        for (const auto& [key, val] : cacheMap)
            cout << key << " => " << val << endl;
    }
};




string normalize(const string& s) {
    string result = s;
    transform(result.begin(), result.end(), result.begin(), ::tolower);
    result.erase(remove_if(result.begin(), result.end(), ::isspace), result.end());
    return result;
}




bool searchCSV(const string& filename, const string& city, const string& code, double& population) {
    ifstream file(filename);
    if (!file.is_open()) {
        cerr << "Error opening file." << endl;
        return false;
    }
    string line, token;
    getline(file, line);



    while (getline(file, line)) {
        stringstream ss(line);
        string fileCode, fileCity;
        double pop;

        getline(ss, fileCode, ',');
        getline(ss, fileCity, ',');
        getline(ss, token, ',');
        pop = stod(token);

        if (normalize(fileCode) == normalize(code) && normalize(fileCity) == normalize(city)) {
            population = pop;
            return true;
        }
    }




    return false;
}

int main() {
    const string filename = "world_cities.csv";
    ICacheStrategy* cache = nullptr;
    int choice;

    cout << "Select Caching Strategy:\n1. LFU\n2. FIFO\n3. Random\nChoice: ";
    cin >> choice;
    cin.ignore();

    switch (choice) {
        case 1: cache = new LFUCache(10); break;
        case 2: cache = new FIFOCache(10); break;
        case 3: cache = new RandomCache(10); break;
        default: cout << "Invalid choice.\n"; return 1;
    }

    string city, code;
    while (true) {
        cout << "\nEnter city name (or type 'exit' to quit): ";
        getline(cin, city);
        if (normalize(city) == "exit") break; 

        cout << "Enter country code: ";
        getline(cin, code);

        string key = normalize(code) + "," + normalize(city);
        double population;




        if (cache->get(key, population)) {
            cout << "[Cache Hit] Population of " << city << ", " << code << " is: " << population << endl;
        } else if (searchCSV(filename, city, code, population)) {
            cout << "[CSV Lookup] Population of " << city << ", " << code << " is: " << population << endl;
            cache->put(key, population);
        } else {
            cout << "City not found in database." << endl;
        }

        cache->printCache();
    }

    delete cache;
    return 0;
}

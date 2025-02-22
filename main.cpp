#include <iostream>
#include <vector>
#include <string>
#include <sstream>
#include <filesystem>
#include <chrono>
#include <thread>
#include <atomic>
#include <mutex>
#include <omp.h>
#include <algorithm>
#include <numeric>
#include "pugixml.hpp"

using namespace std;
using namespace pugi;
namespace fs = std::filesystem;

atomic<int> filesScanned(0);
atomic<bool> done(false);
mutex mtx;

void searchMatchingFiles(const char* xmlFile, const vector<int>& selectedFields, const vector<string>& fields, const vector<string>& searchValues, vector<string>& matchingFiles) {
    xml_document doc;
    xml_parse_result result = doc.load_file(xmlFile);

    if (!result) {
        cerr << "Failed to load file: " << xmlFile << endl;
        return;
    }

    xml_node root = doc.child("stockItem");
    if (!root) {
        cerr << "No root element found in the XML file." << endl;
        return;
    }

    for (xml_node item = root.child("item"); item; item = item.next_sibling("item")) {
        bool match = true;
        for (size_t i = 0; i < selectedFields.size(); ++i) {
            int index = selectedFields[i];
            if (string(item.child_value(fields[index].c_str())) != searchValues[i]) {
                match = false;
                break;
            }
        }
        if (match) {
            lock_guard<mutex> lock(mtx);
            matchingFiles.push_back(xmlFile);
            break;
        }
    }
}

void displayProgress(chrono::time_point<chrono::high_resolution_clock> start, vector<int>& filesProcessed, vector<vector<double>>& threadDurations) {
    while (!done) {
        auto now = chrono::high_resolution_clock::now();
        chrono::duration<double> duration = now - start;
        cout << "\033[2J\033[H"; // Clear screen and move cursor to top-left
        cout << "Scanned " << filesScanned << " files. Time elapsed: " << duration.count() << " seconds." << endl;

        for (size_t i = 0; i < filesProcessed.size(); ++i) {
            if (!threadDurations[i].empty()) {
                sort(threadDurations[i].begin(), threadDurations[i].end());
                double medianDuration = threadDurations[i][threadDurations[i].size() / 2];
                cout << "Thread " << i << " processed " << filesProcessed[i] << " files with a median time of " << medianDuration << " seconds per file." << endl;
            }
        }

        this_thread::sleep_for(chrono::seconds(1));
    }
}

void searchFilesMultithreaded(const string& xmlFolder, const vector<int>& selectedFields, const vector<string>& fields, const vector<string>& searchValues, vector<string>& matchingFiles, int numThreads) {
    auto searchStart = chrono::high_resolution_clock::now();
    vector<int> filesProcessed(numThreads, 0);
    vector<vector<double>> threadDurations(numThreads);
    thread progressThread(displayProgress, searchStart, ref(filesProcessed), ref(threadDurations));

    vector<fs::path> xmlFiles;
    for (const auto& entry : fs::directory_iterator(xmlFolder)) {
        if (entry.path().extension() == ".xml") {
            xmlFiles.push_back(entry.path());
        }
    }

    #pragma omp parallel for num_threads(numThreads) schedule(dynamic)
    for (size_t i = 0; i < xmlFiles.size(); ++i) {
        int threadNum = omp_get_thread_num();
        auto threadStart = chrono::high_resolution_clock::now();

        filesScanned++;
        vector<string> localMatchingFiles;
        searchMatchingFiles(xmlFiles[i].string().c_str(), selectedFields, fields, searchValues, localMatchingFiles);
        if (!localMatchingFiles.empty()) {
            lock_guard<mutex> lock(mtx);
            matchingFiles.insert(matchingFiles.end(), localMatchingFiles.begin(), localMatchingFiles.end());
        }

        auto threadEnd = chrono::high_resolution_clock::now();
        chrono::duration<double> threadDuration = threadEnd - threadStart;
        filesProcessed[threadNum]++;
        threadDurations[threadNum].push_back(threadDuration.count());
    }

    done = true;
    progressThread.join();

    auto searchEnd = chrono::high_resolution_clock::now();
    chrono::duration<double> searchDuration = searchEnd - searchStart;

    cout << "\nTime taken for multithreaded search: " << searchDuration.count() << " seconds." << endl;

    for (int i = 0; i < numThreads; ++i) {
        if (!threadDurations[i].empty()) {
            sort(threadDurations[i].begin(), threadDurations[i].end());
            double medianDuration = threadDurations[i][threadDurations[i].size() / 2];
            cout << "Thread " << i << " processed " << filesProcessed[i] << " files with a median time of " << medianDuration << " seconds per file." << endl;
        }
    }
}

int main() {
    string xmlFolder = "./xml_files/";
    string xmlOutFolder = "./XMLs_out/";
    string xmlFileName;

    // Ensure the output folder exists
    if (!fs::exists(xmlOutFolder)) {
        fs::create_directory(xmlOutFolder);
    }

    cout << "Enter the name of the XML file to process: ";
    cin >> xmlFileName;

    string xmlFilePath = xmlFolder + xmlFileName;

    if (!fs::exists(xmlFilePath)) {
        cerr << "The file " << xmlFilePath << " does not exist." << endl;
        return 1;
    }

    xml_document doc;
    xml_parse_result result = doc.load_file(xmlFilePath.c_str());

    if (!result) {
        cerr << "Failed to load file: " << xmlFilePath << endl;
        return 1;
    }

    xml_node root = doc.child("stockItem");
    if (!root) {
        cerr << "No root element found in the XML file." << endl;
        return 1;
    }

    vector<string> fields;
    for (xml_node item = root.child("item"); item; item = item.next_sibling("item")) {
        for (xml_node field = item.first_child(); field; field = field.next_sibling()) {
            string fieldName = field.name();
            if (find(fields.begin(), fields.end(), fieldName) == fields.end()) {
                fields.push_back(fieldName);
            }
        }
    }

    cout << "Searchable fields in the XML file:" << endl;
    for (size_t i = 0; i < fields.size(); ++i) {
        cout << i + 1 << ". " << fields[i] << endl;
    }

    cout << "Enter the numbers of the fields to search (separated by commas): ";
    string input;
    cin >> input;
    stringstream ss(input);
    string token;
    vector<int> selectedFields;
    while (getline(ss, token, ',')) {
        selectedFields.push_back(stoi(token) - 1);
    }

    vector<string> searchValues(selectedFields.size());
    cin.ignore(); // Ignore the newline character left in the input buffer
    for (size_t i = 0; i < selectedFields.size(); ++i) {
        cout << "Enter the value to search for in field '" << fields[selectedFields[i]] << "': ";
        getline(cin, searchValues[i]);
    }

    vector<string> matchingFiles;
    int filesCopied = 0;

    cout << "Do you want to use multithreaded processing? (y/n): ";
    char useMultithread;
    cin >> useMultithread;

    vector<int> filesProcessed;
    vector<vector<double>> threadDurations;

    if (useMultithread == 'y' || useMultithread == 'Y') {
        int numThreads = omp_get_max_threads();
        cout << "Available threads: " << numThreads << endl;
        cout << "Enter the number of threads to use: ";
        cin >> numThreads;

        filesProcessed.resize(numThreads, 0);
        threadDurations.resize(numThreads);

        searchFilesMultithreaded(xmlFolder, selectedFields, fields, searchValues, matchingFiles, numThreads);
    } else {
        auto searchStart = chrono::high_resolution_clock::now();
        filesProcessed.resize(1, 0);
        threadDurations.resize(1);
        thread progressThread(displayProgress, searchStart, ref(filesProcessed), ref(threadDurations));

        for (const auto& entry : fs::directory_iterator(xmlFolder)) {
            if (entry.path().extension() == ".xml") {
                filesScanned++;
                auto threadStart = chrono::high_resolution_clock::now();

                searchMatchingFiles(entry.path().string().c_str(), selectedFields, fields, searchValues, matchingFiles);

                auto threadEnd = chrono::high_resolution_clock::now();
                chrono::duration<double> threadDuration = threadEnd - threadStart;
                filesProcessed[0]++;
                threadDurations[0].push_back(threadDuration.count());
            }
        }

        done = true;
        progressThread.join();

        auto searchEnd = chrono::high_resolution_clock::now();
        chrono::duration<double> searchDuration = searchEnd - searchStart;

        cout << "\nTime taken for search: " << searchDuration.count() << " seconds." << endl;

        if (!threadDurations[0].empty()) {
            sort(threadDurations[0].begin(), threadDurations[0].end());
            double medianDuration = threadDurations[0][threadDurations[0].size() / 2];
            cout << "Thread 0 processed " << filesProcessed[0] << " files with a median time of " << medianDuration << " seconds per file." << endl;
        }
    }

    for (const string& file : matchingFiles) {
        fs::copy(file, xmlOutFolder + fs::path(file).filename().string(), fs::copy_options::overwrite_existing);
        filesCopied++;
    }

    cout << "\nScanned " << filesScanned << " files." << endl;
    cout << "Copied " << filesCopied << " matching files to " << xmlOutFolder << endl;

    return 0;
}
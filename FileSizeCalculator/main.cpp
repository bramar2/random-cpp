
#include <fstream>
#include <iostream>
#include <filesystem>
#include <string>
#include <vector>
#include <algorithm>
#include <numeric>
#include <iomanip>
#include <cmath>
#include <unordered_map>
#include <random>

using namespace std;
namespace fs = std::filesystem;

const unsigned long long GB_BYTES = 1024*1024*1024;
const unsigned long long MB_BYTES = 1024*1024;
const unsigned long long KB_BYTES = 1024;
const unsigned long long ULL_MAX = numeric_limits<unsigned long long>::max();

string size_repr(unsigned long long bytes) {
    stringstream out;
    out << fixed << setprecision(4);
    if(bytes >= GB_BYTES) {
        if(bytes % GB_BYTES == 0) {
            out << (bytes / GB_BYTES) << " GB";
        }else {
            out << ((double) bytes / GB_BYTES) << " GB";
        }
    }else if(bytes >= MB_BYTES) {
        if(bytes % MB_BYTES == 0) {
            out << (bytes / MB_BYTES) << " MB";
        }else {
            out << ((double) bytes / MB_BYTES) << " MB";
        }
    }else if(bytes >= KB_BYTES) {
        if(bytes % KB_BYTES == 0) {
            out << (bytes / KB_BYTES) << " KB";
        }else {
            out << ((double) bytes / KB_BYTES) << " KB";
        }
    }else {
        out << bytes << " B";
    }
    return out.str();
}

unsigned long long recursiveCalc(vector<pair<fs::path, unsigned long long>>& fileSizes,
                   vector<pair<fs::path, unsigned long long>>& folderSizes,
                   vector<pair<fs::path, unsigned long long>>& folderSizesPure,
                   size_t& longestPathName,
                   const fs::path& root,
                   const fs::path& path) {
    unsigned long long sizeWithFolders = 0;
    unsigned long long sizeNoFolders = 0;
    fs::directory_iterator directoryIterator;
    try {
        directoryIterator = fs::directory_iterator(path);
    }catch(const fs::filesystem_error& err) {
        cerr << "\rerr directory_iterator " << absolute(path) << ": " << err.what() << "\n\r" << flush;
        return 0;
    }
    for(const fs::directory_entry& entry : directoryIterator) {
        if(entry.is_directory()) {
            sizeWithFolders += recursiveCalc(fileSizes,
                                             folderSizes, folderSizesPure, longestPathName, root, entry.path());
        }else {
            unsigned long long fileSize = 0ull;
            try {
                fileSize = static_cast<unsigned long long>(entry.file_size());
            }catch(const fs::filesystem_error& err) {
                cerr << "\rerr file_size " << absolute(entry.path()) << ": " << err.what() << "\n\r" << flush;
            }
            sizeWithFolders += fileSize;
            sizeNoFolders += fileSize;

            fileSizes.emplace_back(entry.path(), fileSize);

            cout << "\rCalculating... " << fileSizes.size() << " files, " << folderSizes.size() << " directories " << flush;
            if(entry.path().string().size() > longestPathName) {
                longestPathName = entry.path().string().size();
            }
        }
    }
    folderSizes.emplace_back(path, sizeWithFolders);
    folderSizesPure.emplace_back(path, sizeNoFolders);
    cout << "\rCalculating... " << fileSizes.size() << " files, " << folderSizes.size() << " directories " << flush;
    //Size representation
    return sizeWithFolders;
}
void parseDivisions(string& input, vector<unsigned long long>& output) {
    if(input.empty()) return;
    transform(input.begin(), input.end(),
              input.begin(), ::tolower);

    // split by ','
    size_t start = 0;
    size_t delimiterPos; // delimiter is ','
    while(true) {
        bool breakAfterThis = false;
        if((delimiterPos = input.find(',', start)) == string::npos) {
            breakAfterThis = true;
            delimiterPos = input.size(); // -1 for right
        }
        size_t left = start;
        size_t right = delimiterPos - 1;
//        size < 2
//       right - left + 1 < 2
//       right - left < 1
        if(right - left < 1) {
            // not enough size
            throw invalid_argument("invalid division length '" + input.substr(left, delimiterPos-left) + "'");
        }
        // do something with left -> right
        // check suffix
        if(input[right] != 'b') { // has to be! GB,MB,KB,B
            throw invalid_argument("invalid division suffix '" + input.substr(left, delimiterPos-left) + "'");
        }

        char secondLast = input[right-1];
        unsigned long long multiplier;
        if(secondLast == 'g') {
            multiplier = GB_BYTES;
            right -= 2;
        }else if(secondLast == 'm') {
            multiplier = MB_BYTES;
            right -= 2;
        }else if(secondLast == 'k') {
            multiplier = KB_BYTES;
            right -= 2;
        }else {
            // byte?
            multiplier = 1;
            right--;
            if(!('0' <= input[right] && input[right] <= '9')) {
                // not a Byte
                throw invalid_argument("invalid division size type '" + input.substr(left, delimiterPos-left) + "'");
            }
        }
        if(left > right) {
            throw invalid_argument("invalid division length after suffix '" + input.substr(left, delimiterPos-left) + "'");
        }
        // convert substr left->right into ull
        long double value = stold(input.substr(left,right+1-left));
        long double bytes = value * multiplier;
        if(bytes >= ULL_MAX) {
            throw invalid_argument("division size above unsigned long long max '" + input.substr(left, delimiterPos-left) + "'");
        }
        output.push_back(static_cast<long long>(bytes));
        start = delimiterPos+1;
        if(breakAfterThis) break;
    }
}
int main() {
    cout << "Enter folder: ";
    string sortedOutput, folderStr;
    getline(cin, folderStr);
    cout << "Output to: ";
    getline(cin, sortedOutput);

    fs::path folderPath = folderStr;
    if(!fs::exists(folderPath)) {
        cerr << "Error folder does not exist: " << folderStr << endl;
        return 1;
    }
    if(!fs::is_directory(folderPath)) {
        cerr << "Error path is not a directory: " << folderStr << endl;
        return 2;
    }
    if(!fs::absolute(static_cast<fs::path>(sortedOutput)).has_parent_path()) {
        cerr << "Error sortedOutput has no directory: " << folderStr << endl;
        return 4;
    }

    unsigned long long fSize;
    vector<pair<fs::path, unsigned long long>> fileSizes, folderSizes, folderSizesPure;
    vector<unsigned long long> fileDivisions, folderDivisions;
    {
        try {
            string divisionInput;
            cout << "Write divisions for files (ex: '' for none, '10gb,1gb,500b' will >10GB,>1GB): " << flush;
            getline(cin, divisionInput);
            parseDivisions(divisionInput, fileDivisions);
            cout << "Write divisions for folders: " << flush;
            getline(cin, divisionInput);
            parseDivisions(divisionInput, folderDivisions);
            sort(fileDivisions.begin(), fileDivisions.end(), greater<>());
            sort(folderDivisions.begin(), folderDivisions.end(), greater<>());
        }catch(const invalid_argument& err) {
            cerr << "Failed to parse division: " << err.what() << endl;
            return 3;
        }
    }
    size_t longestPathNameSizeT = 0;
    {
        cout << "Calculating files... \r" << flush;
        fSize = recursiveCalc(fileSizes, folderSizes, folderSizesPure, longestPathNameSizeT, folderPath, folderPath);
    }
    longestPathNameSizeT += 20;
    int longestPathName;
    if(longestPathNameSizeT >= numeric_limits<int>::max()) {
        cout << "Warning: longest path name used for sorting output (" << longestPathNameSizeT << ") is above int limit, so using int max ("
            << numeric_limits<int>::max() << ")";
        longestPathName = numeric_limits<int>::max();
    }else {
        longestPathName = static_cast<int>(longestPathNameSizeT);
    }
    cout << "\nFinished calculating " << fileSizes.size() << " files, " << folderSizes.size() << " directories" << endl;
    unsigned long long fileLine = 0, folderAllLine = 0, folderPureLine = 0;
    vector<pair<int,int>> fileDivisionLines, folderAllDivisionLines, folderPureDivisionLines;
    {
        cout << "\nSorting fileSizes..." << flush;
        sort(fileSizes.begin(), fileSizes.end(),
             [](pair<fs::path, unsigned long long>& a, pair<fs::path, unsigned long long>& b) -> bool {
            return a.second > b.second; // Reverse order biggest to smallest
        });
        cout << "\nSorting folderSizes..." << flush;
        sort(folderSizes.begin(), folderSizes.end(),
             [](pair<fs::path, unsigned long long>& a, pair<fs::path, unsigned long long>& b) -> bool {
                 return a.second > b.second; // Reverse order biggest to smallest
             });
        cout << "\nSorting folderSizesPure..." << flush;
        sort(folderSizesPure.begin(), folderSizesPure.end(),
             [](pair<fs::path, unsigned long long>& a, pair<fs::path, unsigned long long>& b) -> bool {
                 return a.second > b.second; // Reverse order biggest to smallest
             });
        // Sorted

        cout << "\nWriting... " << flush;
        ofstream sortedOut(fs::absolute(sortedOutput));
        // Sorted Output X
        //
        // Files: Line X
        // Folders S: Line X
        // Folders Y : line x
        //
        // ==FILES START==
        int lineCount = 1;
        sortedOut << "Sorted Output " << fs::absolute(folderPath) << " [" << size_repr(fSize) << "]\n\n"; lineCount += 2;
//        sortedOut << "Files: Line X\n"; lineCount++;
//        sortedOut << "Folders sort on all: Line X\n"; lineCount ++;
//        sortedOut << "Folders sort on pure: Line X\n\n"; lineCount += 2; // this will be updated later along with divisions
        sortedOut << '#';
        lineCount++;
        lineCount++;
        lineCount += 2;
        sortedOut << '\n'; lineCount++;
        cout << "\rWriting... files... " << flush;
        { // Files
            sortedOut << "==== FILES START====\n";
            fileLine = lineCount;
            lineCount++;
            int totalRankDigits = static_cast<int>(ceil(log10(fileSizes.size()))) + 3;
            // +3 for '. ' and 1 more for error
            sortedOut << setw(totalRankDigits) << left << "Rank";
            sortedOut << setw(longestPathName) << left << "File";
            sortedOut << setw(20) << left << "Size";
            sortedOut << '\n'; lineCount++;
            size_t currentIndex = 0;
            size_t filesCount = fileSizes.size();
            for(unsigned long long division : fileDivisions) {
                sortedOut << "Marker Start " << size_repr(division) << '\n';
                int divisionLineStart = lineCount;
                lineCount++;
                // Go through all files that match
                while(currentIndex < filesCount) {
                    if(fileSizes[currentIndex].second >= division) {
                        sortedOut << setw(totalRankDigits) << left << (to_string(currentIndex+1) + ". ");
                        sortedOut << setw(longestPathName) << left << ("\'" + fileSizes[currentIndex].first.string() + "\'");
                        sortedOut << setw(20) << left << (size_repr(fileSizes[currentIndex].second));
                        sortedOut << '\n'; lineCount++;
                        currentIndex++;
                    }else break;
                }
                sortedOut << "Marker End " << size_repr(division) << '\n';
                int divisionLineEnd = lineCount;
                lineCount++;
                fileDivisionLines.emplace_back(divisionLineStart, divisionLineEnd);
            }
            // go through the rest of files
            while(currentIndex < filesCount) {
                sortedOut << setw(totalRankDigits) << left << (to_string(currentIndex+1) + ". ");
                sortedOut << setw(longestPathName) << left << ("\'" + fileSizes[currentIndex].first.string() + "\'");
                sortedOut << setw(20) << left << (size_repr(fileSizes[currentIndex].second));
                sortedOut << '\n'; lineCount++;
                currentIndex++;
            }
        }
        cout << "\rWriting... folders full..." << flush;
        { // Folders all
            sortedOut << '\n'; lineCount++;
            sortedOut << "==== FOLDERS FULL START ====\n";
            folderAllLine = lineCount;
            lineCount++;

            int totalRankDigits = static_cast<int>(ceil(log10(folderSizes.size()))) + 3;
            sortedOut << setw(totalRankDigits) << left << "Rank";
            sortedOut << setw(longestPathName) << left << "Folder";
            sortedOut << setw(20) << left << "Size (Full)";
            sortedOut << setw(20) << left << "Size (Pure)";
            sortedOut << '\n'; lineCount++;

            unordered_map<fs::path, unsigned long long> pureSizesMap;
            pureSizesMap.reserve(folderSizesPure.size());
            for(auto& pair : folderSizesPure) {
                pureSizesMap.insert(pair);
            }

            size_t currentIndex = 0;
            size_t foldersCount = folderSizes.size();
            for(unsigned long long division : folderDivisions) {
                sortedOut << "Marker Start " << size_repr(division) << '\n';
                int divisionLineStart = lineCount;
                lineCount++;
                // Go through all files that match
                while(currentIndex < foldersCount) {
                    if(folderSizes[currentIndex].second >= division) {
                        sortedOut << setw(totalRankDigits) << left << (to_string(currentIndex+1) + ". ");
                        sortedOut << setw(longestPathName) << left << ("\'" + folderSizes[currentIndex].first.string() + "\'");
                        sortedOut << setw(20) << left << (size_repr(folderSizes[currentIndex].second));
                        sortedOut << setw(20) << left << (size_repr(pureSizesMap[folderSizes[currentIndex].first]));
                        sortedOut << '\n'; lineCount++;
                        currentIndex++;
                    }else break;
                }
                sortedOut << "Marker End " << size_repr(division) << '\n';
                int divisionLineEnd = lineCount;
                lineCount++;
                folderAllDivisionLines.emplace_back(divisionLineStart, divisionLineEnd);
            }
            // go through the rest of folders
            while(currentIndex < foldersCount) {
                sortedOut << setw(totalRankDigits) << left << (to_string(currentIndex+1) + ". ");
                sortedOut << setw(longestPathName) << left << ("\'" + folderSizes[currentIndex].first.string() + "\'");
                sortedOut << setw(20) << left << (size_repr(folderSizes[currentIndex].second));
                sortedOut << setw(20) << left << (size_repr(pureSizesMap[folderSizes[currentIndex].first]));
                sortedOut << '\n'; lineCount++;
                currentIndex++;
            }
        }
        cout << "\rWriting... folders pure..." << flush;
        { // Folders pure
            sortedOut << '\n'; lineCount++;
            sortedOut << "==== FOLDERS PURE START ====\n";
            folderPureLine = lineCount;
            lineCount++;

            int totalRankDigits = static_cast<int>(ceil(log10(folderSizesPure.size()))) + 3;
            sortedOut << setw(totalRankDigits) << left << "Rank";
            sortedOut << setw(longestPathName) << left << "Folder";
            sortedOut << setw(20) << left << "Size (Full)";
            sortedOut << setw(20) << left << "Size (Pure)";
            sortedOut << '\n'; lineCount++;

            unordered_map<fs::path, unsigned long long> allSizesMap;
            allSizesMap.reserve(folderSizes.size());
            for(auto& pair : folderSizes) {
                allSizesMap.insert(pair);
            }

            size_t currentIndex = 0;
            size_t foldersCount = folderSizesPure.size();
            for(unsigned long long division : folderDivisions) {
                sortedOut << "Marker Start " << size_repr(division) << '\n';
                int divisionLineStart = lineCount;
                lineCount++;
                // Go through all files that match
                while(currentIndex < foldersCount) {
                    if(folderSizesPure[currentIndex].second >= division) {
                        sortedOut << setw(totalRankDigits) << left << (to_string(currentIndex+1) + ". ");
                        sortedOut << setw(longestPathName) << left << ("\'" + folderSizesPure[currentIndex].first.string() + "\'");
                        sortedOut << setw(20) << left << (size_repr(allSizesMap[folderSizesPure[currentIndex].first]));
                        sortedOut << setw(20) << left << (size_repr(folderSizesPure[currentIndex].second));
                        sortedOut << '\n'; lineCount++;
                        currentIndex++;
                    }else break;
                }
                sortedOut << "Marker End " << size_repr(division) << '\n';
                int divisionLineEnd = lineCount;
                lineCount++;
                folderPureDivisionLines.emplace_back(divisionLineStart, divisionLineEnd);
            }
            // go through the rest of folders
            while(currentIndex < foldersCount) {
                sortedOut << setw(totalRankDigits) << left << (to_string(currentIndex+1) + ". ");
                sortedOut << setw(longestPathName) << left << ("\'" + folderSizesPure[currentIndex].first.string() + "\'");
                sortedOut << setw(20) << left << (size_repr(allSizesMap[folderSizesPure[currentIndex].first]));
                sortedOut << setw(20) << left << (size_repr(folderSizesPure[currentIndex].second));
                sortedOut << '\n'; lineCount++;
                currentIndex++;
            }
        }
        cout << "\rWriting... markers and lines..." << endl;
        sortedOut << flush;
    }
    {
        // Edit
        stringstream lineContent;

        lineContent << "Files: L" << fileLine << " | ";
        for(size_t i = 0; i < fileDivisions.size(); i++) {
            pair<int,int>& startEnd = fileDivisionLines[i];
            lineContent << size_repr(fileDivisions[i]) <<
                    ": L" << startEnd.first << " - L" << startEnd.second << " | ";
        }
        char n = 10;
        lineContent.write(&n, 1);
        lineContent << "Folders fullsort: L" << folderAllLine << " | ";
        for(size_t i = 0; i < folderDivisions.size(); i++) {
            pair<int,int>& startEnd = folderAllDivisionLines[i];
            lineContent << size_repr(folderDivisions[i]) <<
                        ": L" << startEnd.first << " - L" << startEnd.second << " | ";
        }
        lineContent << '\n';
        lineContent << "Folders puresort: L" << folderPureLine << " | ";
        for(size_t i = 0; i < folderDivisions.size(); i++) {
            pair<int,int>& startEnd = folderPureDivisionLines[i];
            lineContent << size_repr(folderDivisions[i]) <<
                        ": L" << startEnd.first << " - L" << startEnd.second << " | ";
        }
        lineContent << "\n\n";
        string replacement = lineContent.str();

        // replace
        fs::path sortedOutputPath = fs::absolute(sortedOutput);

        std::random_device rd;
        std::mt19937 gen(rd());
        std::uniform_int_distribution<long long> dist(1ll, 600000000000000ll);
        fs::path tempFile(sortedOutputPath.parent_path());
        tempFile /= "test.txt";
        do {
            tempFile = tempFile.parent_path();
            tempFile /= ("~fsc-temps-" + to_string(dist(gen)+2) + to_string(dist(gen)));
        }while(fs::exists(tempFile));

        {
            ofstream tempStream(tempFile);
            ifstream in(sortedOutputPath);
            char read;
            while(true) {
                in.read(&read, 1);
                if(read == -1 || read == '#') {
                    break;
                }
                tempStream << read;
            }
            if(read == -1) {
                // ????
                tempStream.close();
                if(!fs::remove(tempFile)) {
                    cerr << "\nWarning: failed to remove tempFile '" << tempFile << '\'' << endl;
                }
            }else {
                // '#' lets gooo
                tempStream << replacement;
                // pipe everything else
                char buffer[1024];
                while(in.read(buffer, 1024)) {
                    tempStream.write(buffer, in.gcount());
                }
                // succcees!
                tempStream.close();
                in.close();
                if(!fs::remove(sortedOutputPath)) {
                    cerr << "\nWarning: failed to remove original sorted file '" << sortedOutputPath << '\'' << endl;
                    cerr << "\nYou can view the full sorted output in '" << tempFile << '\'' << endl;
                }else {
                    fs::rename(tempFile, sortedOutputPath);
                }
            }
        }
    }
    cout << "\rDone\n\nFinished, press [ENTER] to exit!" << endl;
    string a;
    getline(cin, a);
    return 0;
}

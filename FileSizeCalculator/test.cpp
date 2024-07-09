#include <iostream>
#include <string>
#include <iomanip>
#include <fstream>
#include <filesystem>
using namespace std;
namespace fs = std::filesystem;
int main() {
    cout << fs::absolute("sortedOutput.txt").string() << endl;
}
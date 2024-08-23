#include <iostream>
#include <fstream>
#include <string>
#include <unordered_map>
#include <vector>

using namespace std;

void parseAuthLog(const std::string &filePath, int &totalFailures) {
    std::ifstream logFile(filePath);
    std::string line;
    std::unordered_map<std::string, int> userFailures;

    if (logFile.is_open()) {
        while (std::getline(logFile, line)) {
            std::size_t pos = line.find("Failed password for");

            if (pos != std::string::npos) {
                std::string user = line.substr(pos + 20);
                user = user.substr(0, user.find(" ")); // Extract the username

                // Increment the failure count for the user
                if (userFailures.find(user) == userFailures.end()) {
                    userFailures[user] = 1;
                } else {
                    userFailures[user]++;
                }

                // Increment the total failures count
                totalFailures++;
            }
        }

        logFile.close();
    } else {
        std::cerr << "Unable to open file" << std::endl;
    }

    // Display the failure attempts for each user
    for (const auto &pair : userFailures) {
        std::cout << "User: " << pair.first << " - Failed Attempts: " << pair.second << std::endl;
    }
}

int main() {
    std::string logFilePath = "auth.log";
    int totalFailures = 0;

    parseAuthLog(logFilePath, totalFailures);

    std::cout << "Total Failed Attempts: " << totalFailures << std::endl;

    return 0;
}


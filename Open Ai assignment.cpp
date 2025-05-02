// Open AI assignment CIT-66
// Alex lopez
// 5/2/2025

#include <iostream>
#include <string>
#include <vector>
#include <chrono>
#include <thread>
#include <curl/curl.h>
#include "json.hpp"


using json = nlohmann::json;
using namespace std;
using namespace std::chrono;

size_t WriteCallback(void* contents, size_t size, size_t nmemb, string* out) {
    size_t totalSize = size * nmemb;
    out->append((char*)contents, totalSize);
    return totalSize;
}

string getTimeInItaly() {
    string responseString;
    CURL* curl = curl_easy_init();

    if (curl) {
        string url = "https://worldtimeapi.org/api/timezone/Europe/Rome";
        curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
        curl_easy_setopt(curl, CURLOPT_CAINFO, "C:/Users/muril/source/repos/Open Ai assignment/Open Ai assignment/cacert.pem");
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &responseString);
        curl_easy_setopt(curl, CURLOPT_CAINFO, "C:/2024_Fall/cit66_Cpp/chatBot01/cacert.pem");
        curl_easy_setopt(curl, CURLOPT_VERBOSE, 1L);


        CURLcode res = curl_easy_perform(curl);
        if (res != CURLE_OK) {
            cerr << "cURL error: " << curl_easy_strerror(res) << endl;
        }

        curl_easy_cleanup(curl);
    }

    if (responseString.empty()) {
        return "Error: Could not fetch the time. Please try again later.";
    }

    try {
        json jsonResponse = json::parse(responseString);
        if (jsonResponse.contains("datetime")) {
            return jsonResponse["datetime"];
        }
        else {
            return "Error: Unexpected response format from WorldTimeAPI.";
        }
    }
    catch (...) {
        return "Error: Failed to parse response from WorldTimeAPI.";
    }
}

string sendMessageToChatbotRecursive(const string& userMessage, const string& apiKey, int attempt = 1, int maxRetries = 3) {
    string responseString;
    CURL* curl = curl_easy_init();

    if (curl) {
        string url = "https://api.openai.com/v1/chat/completions";
        string payload = R"({
            "model": "gpt-3.5-turbo",
            "messages": [{"role": "user", "content": ")" + userMessage + R"("}]
        })";

        struct curl_slist* headers = nullptr;
        headers = curl_slist_append(headers, ("Authorization: Bearer " + apiKey).c_str());
        headers = curl_slist_append(headers, "Content-Type: application/json");

        curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
        curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
        curl_easy_setopt(curl, CURLOPT_POSTFIELDS, payload.c_str());
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &responseString);
        curl_easy_setopt(curl, CURLOPT_CAINFO, "C:/2024_Fall/cit66_Cpp/chatBot01/cacert.pem");

        CURLcode res = curl_easy_perform(curl);
        curl_easy_cleanup(curl);
        curl_slist_free_all(headers);

        if (res != CURLE_OK) {
            cerr << "Attempt " << attempt << " failed: " << curl_easy_strerror(res) << endl;
            if (attempt < maxRetries) {
                this_thread::sleep_for(seconds(2));
                return sendMessageToChatbotRecursive(userMessage, apiKey, attempt + 1, maxRetries);
            }
            else {
                return "Error: Failed to get a response after several attempts.";
            }
        }
    }

    return responseString;
}

int main() {
    string apiKey = "sk-REDACTED"; // Replace with your real API key
    string userMessage;
    string chatbotName = "Assistant";
    string userName = "User";

    int exchangeCount = 0;
    vector<pair<string, string>> history;
    vector<double> responseTimes;

    cout << "Chatbot (type 'exit' to quit):\n";

    while (true) {
        cout << "> ";
        getline(cin, userMessage);

        // Input validation
        if (userMessage.empty()) {
            cout << "Input cannot be empty. Please try again.\n";
            continue;
        }
        if (userMessage.length() > 300) {
            cout << "Input exceeds 300 characters. Please shorten your message.\n";
            continue;
        }

        if (userMessage == "exit") break;

        if (userMessage.find("Your name is now") != string::npos) {
            chatbotName = userMessage.substr(17);
            chatbotName.erase(chatbotName.find_last_not_of(" \n\r\t") + 1);
            cout << "Bot: Okay, I will now call myself " << chatbotName << ".\n";
            continue;
        }

        if (userMessage.find("my name is") != string::npos) {
            userName = userMessage.substr(10);
            userName.erase(userName.find_last_not_of(" \n\r\t") + 1);
            cout << chatbotName << ": Nice to meet you, " << userName << "!\n";
            continue;
        }

        if (userMessage.find("time in Italy") != string::npos) {
            string timeInItaly = getTimeInItaly();
            cout << chatbotName << ": The current time in Italy is: " << timeInItaly << "\n";
            continue;
        }

        auto start = high_resolution_clock::now();
        string response = sendMessageToChatbotRecursive(userMessage, apiKey);
        auto end = high_resolution_clock::now();

        double durationMs = duration_cast<milliseconds>(end - start).count();
        responseTimes.push_back(durationMs);

        string chatbotReply;
        try {
            json jsonResponse = json::parse(response);
            if (jsonResponse.contains("choices") && !jsonResponse["choices"].empty()) {
                chatbotReply = jsonResponse["choices"][0]["message"]["content"];
            }
            else {
                chatbotReply = "Sorry, I couldn't get a valid response.";
            }
        }
        catch (...) {
            chatbotReply = "Error: JSON parsing failed.";
        }

        exchangeCount++;
        history.push_back({ userMessage, chatbotReply });

        cout << chatbotName << ": " << chatbotReply << "\n";
        cout << "Exchange #" << exchangeCount << " | Response Time: " << durationMs << " ms\n";

        if (!responseTimes.empty()) {
            double sum = 0;
            for (double t : responseTimes) sum += t;
            cout << "Average Response Time: " << sum / responseTimes.size() << " ms\n";
        }

        cout << "\n--- Conversation History ---\n";
        for (size_t i = 0; i < history.size(); ++i) {
            cout << userName << ": " << history[i].first << "\n";
            cout << chatbotName << ": " << history[i].second << "\n";
        }
        cout << "----------------------------\n";
    }

    return 0;
}
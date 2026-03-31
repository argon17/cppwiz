using namespace std;
#include <iostream>
#include <cstring>
#include <string>
#include <map>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <unistd.h>
#include <errno.h>
#include <getopt.h>

const int PORT = 38899;
const int TIMEOUT_SEC = 5;

int createUdpSocket()
{
    int socket_fd = socket(AF_INET, SOCK_DGRAM, 0);
    if (socket_fd < 0)
        throw std::runtime_error("Failed to create UDP socket: " + std::string(strerror(errno)));
    struct timeval tv;
    tv.tv_sec = TIMEOUT_SEC;
    tv.tv_usec = 0;
    setsockopt(socket_fd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));
    return socket_fd;
}

int sendUdpPacket(int socket_fd, const string &message, const string &target_ip, int port)
{
    sockaddr_in target{};
    target.sin_family = AF_INET;
    target.sin_port = htons(port);
    if (inet_pton(AF_INET, target_ip.c_str(), &target.sin_addr) <= 0)
        throw std::runtime_error("Invalid target address: " + target_ip);
    ssize_t sent = sendto(socket_fd, message.c_str(), message.length(), 0,
                          (sockaddr *)&target, sizeof(target));
    if (sent < 0)
        throw std::runtime_error("Failed to send packet: " + std::string(strerror(errno)));
    return sent;
}

string recvResponse(int socket_fd)
{
    char buf[4096] = {};
    ssize_t n = recv(socket_fd, buf, sizeof(buf) - 1, 0);
    if (n < 0)
        return "";
    return string(buf, n);
}

sockaddr_in parseIpAddress(const string &ip_str)
{
    sockaddr_in ip{};
    ip.sin_family = AF_INET;
    if (inet_pton(AF_INET, ip_str.c_str(), &ip.sin_addr) <= 0)
        throw std::runtime_error("Invalid IP address: " + ip_str);
    return ip;
}

// Scene name -> scene ID mapping
const map<string, int> SCENES = {
    {"ocean", 1}, {"romance", 2}, {"sunset", 3}, {"party", 4},
    {"fireplace", 5}, {"cozy", 6}, {"forest", 7}, {"pastel colors", 8},
    {"wake up", 9}, {"bedtime", 10}, {"warm white", 11}, {"daylight", 12},
    {"cool white", 13}, {"night light", 14}, {"focus", 15}, {"relax", 16},
    {"true colors", 17}, {"tv time", 18}, {"plant growth", 19}, {"spring", 20},
    {"summer", 21}, {"fall", 22}, {"deep dive", 23}, {"jungle", 24},
    {"mojito", 25}, {"club", 26}, {"christmas", 27}, {"halloween", 28},
    {"candlelight", 29}, {"golden white", 30}, {"pulse", 31}, {"steampunk", 32}
};

class WizBulb
{
    string ip_str;
    int port;

    string sendAndReceive(const string &message)
    {
        int sock = createUdpSocket();
        sendUdpPacket(sock, message, ip_str, port);
        string response = recvResponse(sock);
        close(sock);
        return response;
    }

    void sendOnly(const string &message)
    {
        int sock = createUdpSocket();
        sendUdpPacket(sock, message, ip_str, port);
        close(sock);
    }

public:
    WizBulb(const string &ip, int p = PORT) : ip_str(ip), port(p) {}

    void turnOn()   { sendOnly("{\"method\":\"setPilot\",\"params\":{\"state\":true}}"); }
    void turnOff()  { sendOnly("{\"method\":\"setPilot\",\"params\":{\"state\":false}}"); }

    void setBrightness(int v) {
        turnOn(); // Ensure bulb is on when setting brightness

        sendOnly("{\"method\":\"setPilot\",\"params\":{\"state\":true,\"dimming\":" + to_string(v) + "}}");
    }

    void setColor(int r, int g, int b) {
        turnOn(); // Ensure bulb is on when setting color

        sendOnly("{\"method\":\"setPilot\",\"params\":{\"state\":true,\"r\":" + to_string(r) +
                 ",\"g\":" + to_string(g) + ",\"b\":" + to_string(b) + ",\"dimming\":100}}");
    }

    void setTemperature(int temp) {
        turnOn(); // Ensure bulb is on when setting temperature

        sendOnly("{\"method\":\"setPilot\",\"params\":{\"state\":true,\"temp\":" + to_string(temp) + "}}");
    }

    void setScene(int scene_id) {
        turnOn(); // Ensure bulb is on when setting scene

        sendOnly("{\"method\":\"setPilot\",\"params\":{\"state\":true,\"sceneId\":" + to_string(scene_id) + "}}");
    }

    string getStatus() {
        return sendAndReceive("{\"method\":\"getPilot\",\"params\":{}}");
    }
};

// Simple JSON value extractor (no full parser dependency)
string extractJson(const string &json, const string &key)
{
    string search = "\"" + key + "\":";
    size_t pos = json.find(search);
    if (pos == string::npos) return "";
    pos += search.size();
    while (pos < json.size() && (json[pos] == ' ')) pos++;
    if (pos >= json.size()) return "";
    if (json[pos] == '"') {
        size_t end = json.find('"', pos + 1);
        return json.substr(pos + 1, end - pos - 1);
    }
    size_t end = pos;
    while (end < json.size() && json[end] != ',' && json[end] != '}') end++;
    return json.substr(pos, end - pos);
}

void printStatus(const string &response)
{
    if (response.empty()) { cerr << "❌ No response from bulb (timeout)\n"; return; }
    string state     = extractJson(response, "state");
    string dimming   = extractJson(response, "dimming");
    string temp      = extractJson(response, "temp");
    string r         = extractJson(response, "r");
    string g         = extractJson(response, "g");
    string b         = extractJson(response, "b");
    string sceneId   = extractJson(response, "sceneId");

    cout << "💡 Bulb status: " << (state == "true" ? "ON" : "OFF") << "\n";
    if (!dimming.empty())  cout << "   Brightness: " << dimming << "%\n";
    if (!temp.empty())     cout << "   Temperature: " << temp << "K\n";
    if (!r.empty())        cout << "   Color: RGB(" << r << "," << g << "," << b << ")\n";
    if (!sceneId.empty()) {
        string scene_name = "scene #" + sceneId;
        for (auto &[name, id] : SCENES)
            if (to_string(id) == sceneId) { scene_name = name; break; }
        cout << "   Scene: " << scene_name << "\n";
    }
}

void printUsage(const char *prog)
{
    cout << "Wiz Bulb Control - CLI Tool\n\n";
    cout << "Usage: " << prog << " --ip <address> [OPTIONS]\n\n";
    cout << "Options:\n";
    cout << "  --ip <address>           Target bulb IP address (required)\n";
    cout << "  --on                     Turn bulb ON\n";
    cout << "  --off                    Turn bulb OFF\n";
    cout << "  --brightness <0-100>     Set brightness\n";
    cout << "  --color <r,g,b>          Set RGB color (e.g., 255,0,0)\n";
    cout << "  --temperature <2200-6500> Set color temperature in Kelvin\n";
    cout << "  --mood <1-32>            Set scene by ID\n";
    cout << "  --scene <name>           Set scene by name\n";
    cout << "  --status                 Get current bulb state\n";
    cout << "  --help                   Show this help\n\n";
    cout << "Available scenes:\n  ";
    int i = 0;
    for (auto &[name, id] : SCENES) { cout << name; if (++i < (int)SCENES.size()) cout << ", "; }
    cout << "\n\nExamples:\n";
    cout << "  " << prog << " --ip 192.168.0.109 --on\n";
    cout << "  " << prog << " --ip 192.168.0.109 --brightness 50\n";
    cout << "  " << prog << " --ip 192.168.0.109 --color 255,0,128\n";
    cout << "  " << prog << " --ip 192.168.0.109 --temperature 3000\n";
    cout << "  " << prog << " --ip 192.168.0.109 --scene romance\n";
    cout << "  " << prog << " --ip 192.168.0.109 --status\n";
}

int main(int argc, char *argv[])
{
    string bulb_ip, command, scene_name;
    int brightness = -1, red = -1, green = -1, blue = -1, temperature = -1, mood_id = -1;

    struct option long_options[] = {
        {"ip",          required_argument, 0, 1},
        {"on",          no_argument,       0, 2},
        {"off",         no_argument,       0, 3},
        {"brightness",  required_argument, 0, 4},
        {"color",       required_argument, 0, 5},
        {"help",        no_argument,       0, 6},
        {"mood",        required_argument, 0, 7},
        {"temperature", required_argument, 0, 8},
        {"scene",       required_argument, 0, 9},
        {"status",      no_argument,       0, 10},
        {0, 0, 0, 0}
    };

    if (argc < 2) { printUsage(argv[0]); return 0; }

    int opt, option_index = 0;
    while ((opt = getopt_long(argc, argv, "", long_options, &option_index)) != -1)
    {
        switch (opt) {
        case 1: bulb_ip = optarg; break;
        case 2: command = "on"; break;
        case 3: command = "off"; break;
        case 4:
            brightness = atoi(optarg);
            if (brightness < 0 || brightness > 100) { cerr << "Error: Brightness must be 0-100\n"; return 1; }
            command = "brightness"; break;
        case 5: {
            if (sscanf(optarg, "%d,%d,%d", &red, &green, &blue) != 3 ||
                red < 0 || red > 255 || green < 0 || green > 255 || blue < 0 || blue > 255) {
                cerr << "Error: Invalid color. Use r,g,b (0-255 each)\n"; return 1;
            }
            command = "color"; break;
        }
        case 6: printUsage(argv[0]); return 0;
        case 7:
            mood_id = atoi(optarg);
            if (mood_id < 1 || mood_id > 32) { cerr << "Error: Mood ID must be 1-32\n"; return 1; }
            command = "mood"; break;
        case 8:
            temperature = atoi(optarg);
            if (temperature < 2200 || temperature > 6500) { cerr << "Error: Temperature must be 2200-6500K\n"; return 1; }
            command = "temperature"; break;
        case 9:
            scene_name = optarg;
            command = "scene"; break;
        case 10: command = "status"; break;
        default: cerr << "Unknown option. Use --help.\n"; return 1;
        }
    }

    if (bulb_ip.empty()) { cerr << "Error: --ip is required\n"; return 1; }
    if (command.empty()) { cerr << "Error: No command given. Use --help.\n"; return 1; }

    try {
        WizBulb bulb(bulb_ip);

        if (command == "on")          { bulb.turnOn();   cout << "✓ Bulb turned ON\n"; }
        else if (command == "off")    { bulb.turnOff();  cout << "✓ Bulb turned OFF\n"; }
        else if (command == "brightness") { bulb.setBrightness(brightness); cout << "✓ Brightness set to " << brightness << "%\n"; }
        else if (command == "color")  { bulb.setColor(red, green, blue); cout << "✓ Color set to RGB(" << red << "," << green << "," << blue << ")\n"; }
        else if (command == "temperature") { bulb.setTemperature(temperature); cout << "✓ Temperature set to " << temperature << "K\n"; }
        else if (command == "mood")   { bulb.setScene(mood_id); cout << "✓ Scene ID " << mood_id << " activated\n"; }
        else if (command == "scene") {
            auto it = SCENES.find(scene_name);
            if (it == SCENES.end()) { cerr << "❌ Unknown scene '" << scene_name << "'. Use --help for list.\n"; return 1; }
            bulb.setScene(it->second);
            cout << "✓ Scene '" << scene_name << "' activated\n";
        }
        else if (command == "status") { printStatus(bulb.getStatus()); }

        return 0;
    } catch (const std::exception &e) {
        cerr << "Error: " << e.what() << "\n";
        return 1;
    }
}

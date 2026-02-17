#include <iostream>
#include <cstring>
#include <string>
#include <errno.h>

#ifdef _WIN32
  #include <winsock2.h>
  #include <ws2tcpip.h>
  #pragma comment(lib, "ws2_32.lib")
#else
  #include <arpa/inet.h>
  #include <sys/socket.h>
  #include <unistd.h>
  #include <getopt.h>
#endif

using namespace std;

#ifdef _WIN32
  // getopt_long implementation for Windows
  #define no_argument        0
  #define required_argument  1

  static char *optarg = nullptr;
  static int optind = 1;
  static int opterr = 1;

  struct option {
      const char *name;
      int         has_arg;
      int        *flag;
      int         val;
  };

  static int getopt_long(int argc, char *const argv[], const char * /*optstring*/,
                         const struct option *longopts, int *longindex)
  {
      while (optind < argc) {
          const char *arg = argv[optind];
          if (arg[0] != '-' || arg[1] != '-') { optind++; continue; }
          arg += 2;  // skip "--"
          for (int i = 0; longopts[i].name; i++) {
              if (strcmp(arg, longopts[i].name) == 0) {
                  if (longindex) *longindex = i;
                  optind++;
                  if (longopts[i].has_arg == required_argument) {
                      if (optind < argc) { optarg = argv[optind++]; }
                      else { cerr << "Option --" << longopts[i].name << " requires an argument.\n"; return '?'; }
                  }
                  return longopts[i].val;
              }
          }
          cerr << "Unknown option: " << argv[optind] << "\n";
          optind++;
          return '?';
      }
      return -1;
  }
#endif


#ifdef _WIN32
static bool wsaInitialized = false;
static void initWinsock()
{
    if (!wsaInitialized) {
        WSADATA wsaData;
        int result = WSAStartup(MAKEWORD(2, 2), &wsaData);
        if (result != 0) {
            throw std::runtime_error("WSAStartup failed: " + to_string(result));
        }
        wsaInitialized = true;
    }
}
#endif

int createUdpSocket()
{
#ifdef _WIN32
    initWinsock();
#endif
    int socket_fd = (int)socket(AF_INET, SOCK_DGRAM, 0);
    if (socket_fd < 0)
    {
        throw std::runtime_error("Failed to create UDP socket: " +
                                 std::string(strerror(errno)));
    }
    
    return socket_fd;
}


int sendUdpPacket(int socket_fd, const string &message, const string &target_ip, int port)
{
    sockaddr_in target{};
    target.sin_family = AF_INET;
    target.sin_port = htons(port);

    if (inet_pton(AF_INET, target_ip.c_str(), &target.sin_addr) <= 0)
    {
        throw std::runtime_error("Invalid target address: " + target_ip);
    }

#ifdef _WIN32
    int sent_bytes = sendto(socket_fd, message.c_str(), (int)message.length(), 0,
                           (sockaddr *)&target, sizeof(target));
#else
    ssize_t sent_bytes = sendto(socket_fd, message.c_str(), message.length(), 0,
                                (sockaddr *)&target, sizeof(target));
#endif

    if (sent_bytes < 0)
    {
        throw std::runtime_error("Failed to send packet: " +
                                 std::string(strerror(errno)));
    }
    return sent_bytes;
}

sockaddr_in parseIpAddress(const string &ip_str)
{
    sockaddr_in ip{};
    ip.sin_family = AF_INET;
    if (inet_pton(AF_INET, ip_str.c_str(), &ip.sin_addr) <= 0)
    {
        throw std::runtime_error("Invalid IP address: " + ip_str);
    }
    return ip;
}

const int PORT = 38899;
const int TIMEOUT_SEC = 2;

class WizBulb
{
    private:
    int socket_fd = -1;
    sockaddr_in ip;
    int port;

    int ensureSocket()
    {
        if (socket_fd < 0)
        {
            socket_fd = createUdpSocket();
        }
        return socket_fd;
    }

    public:
    WizBulb(const sockaddr_in &ip, int port) : ip(ip), port(port = PORT)
    {
    }

    bool turnOn()
    {
        socket_fd = ensureSocket();
        string message = "{\"method\":\"setPilot\",\"params\":{\"state\":true}}";
        sendUdpPacket(socket_fd, message, inet_ntoa(ip.sin_addr), port);
        return true;
    }

    bool turnOff()
    {
        socket_fd = ensureSocket();
        string message = "{\"method\":\"setPilot\",\"params\":{\"state\":false}}";
        sendUdpPacket(socket_fd, message, inet_ntoa(ip.sin_addr), port);
        return true;
    }

    bool setBrightness(int brightness)
    {
        socket_fd = ensureSocket();
        string message = "{\"method\":\"setPilot\",\"params\":{\"dimming\":" + to_string(brightness) + "}}";
        sendUdpPacket(socket_fd, message, inet_ntoa(ip.sin_addr), port);
        return true;
    }

    bool setColor(int r, int g, int b)
    {
        socket_fd = ensureSocket();
        string message = "{\"method\":\"setPilot\",\"params\":{\"r\":" + to_string(r) + ",\"g\":" + to_string(g) + ",\"b\":" + to_string(b) + "}}";
        sendUdpPacket(socket_fd, message, inet_ntoa(ip.sin_addr), port);
        return true;
    }

    bool setMood(const string &mood)
    {
        socket_fd = ensureSocket();
        string message = "{\"method\":\"setPilot\",\"params\":{\"sceneId\":" + mood + "}}";
        sendUdpPacket(socket_fd, message, inet_ntoa(ip.sin_addr), port);
        return true;
    }
};

void printUsage(const char *program_name)
{
    cout << "Wiz Bulb Control - CLI Tool\n\n";
    cout << "Usage: " << program_name << " --ip <address> [OPTIONS]\n\n";
    cout << "Options:\n";
    cout << "  --ip <address>       Target bulb IP address (required)\n";
    cout << "  --on                 Turn bulb ON\n";
    cout << "  --off                Turn bulb OFF\n";
    cout << "  --brightness <0-100> Set brightness (0-100)\n";
    cout << "  --color <r,g,b>     Set RGB color (e.g., 255,0,0 for red)\n";
    cout << "  --mood <id>          Set mood/scene (1-32)\n";
    cout << "  --help               Show this help message\n\n";
    cout << "Examples:\n";
    cout << "  " << program_name << " --ip 192.168.0.100 --on\n";
    cout << "  " << program_name << " --ip 192.168.0.100 --brightness 75\n";
    cout << "  " << program_name << " --ip 192.168.0.100 --color 255,0,0\n";
    cout << "  " << program_name << " --ip 192.168.0.100 --mood 5\n";
}

int main(int argc, char *argv[])
{
    string bulb_ip;
    string command;
    int brightness = -1;
    int red = -1, green = -1, blue = -1;
    string mood;

    struct option long_options[] = {
        {"ip", required_argument, 0, 1},
        {"on", no_argument, 0, 2},
        {"off", no_argument, 0, 3},
        {"brightness", required_argument, 0, 4},
        {"color", required_argument, 0, 5},
        {"mood", required_argument, 0, 7},
        {"help", no_argument, 0, 6},
        {0, 0, 0, 0}
    };

    int option_index = 0;
    int opt;

    if (argc < 2)
    {
        printUsage(argv[0]);
        return 0;
    }

    while ((opt = getopt_long(argc, argv, "", long_options, &option_index)) != -1)
    {
        switch (opt)
        {
        case 1:  // --ip
            bulb_ip = optarg;
            break;
        case 2:  // --on
            command = "on";
            break;
        case 3:  // --off
            command = "off";
            break;
        case 4:  // --brightness
            brightness = atoi(optarg);
            if (brightness < 0 || brightness > 100)
            {
                cerr << "Error: Brightness must be between 0-100\n";
                return 1;
            }
            command = "brightness";
            break;
        case 5:  // --color
        {
            int result = sscanf(optarg, "%d,%d,%d", &red, &green, &blue);
            if (result != 3 || red < 0 || red > 255 || green < 0 || green > 255 || blue < 0 || blue > 255)
            {
                cerr << "Error: Invalid color format. Use r,g,b (0-255 each)\n";
                return 1;
            }
            command = "color";
            break;
        }
        case 7:  // --mood
        {
            int mood_id = atoi(optarg);
            if (mood_id < 1 || mood_id > 32)
            {
                cerr << "Error: Mood ID must be between 1-32\n";
                return 1;
            }
            mood = optarg;
            command = "mood";
            break;
        }
        case 6:  // --help
            printUsage(argv[0]);
            return 0;
        default:
            cerr << "Unknown option. Use --help for help.\n";
            return 1;
        }
    }

    if (bulb_ip.empty())
    {
        cerr << "Error: IP address is required (--ip option)\n";
        printUsage(argv[0]);
        return 1;
    }

    if (command.empty())
    {
        cerr << "Error: Command is required. Use --help for help.\n";
        return 1;
    }

    try
    {
        sockaddr_in bulb_addr = parseIpAddress(bulb_ip);
        WizBulb bulb(bulb_addr, PORT);

        if (command == "on")
        {
            cout << "✓ Turning ON bulb at " << bulb_ip << endl;
            bulb.turnOn();
        }
        else if (command == "off")
        {
            cout << "✓ Turning OFF bulb at " << bulb_ip << endl;
            bulb.turnOff();
        }
        else if (command == "brightness")
        {
            cout << "✓ Setting brightness to " << brightness << "% on " << bulb_ip << endl;
            bulb.setBrightness(brightness);
        }
        else if (command == "color")
        {
            cout << "✓ Setting color to RGB(" << red << "," << green << "," << blue << ") on " << bulb_ip << endl;
            bulb.setColor(red, green, blue);
        }
        else if (command == "mood")
        {
            cout << "✓ Setting mood " << mood << " on " << bulb_ip << endl;
            bulb.setMood(mood);
        }
        else
        {
            cerr << "Unknown command. Use --help for help.\n";
            return 1;
        }

        return 0;
    }
    catch (const std::exception &e)
    {
        cerr << "Error: " << e.what() << endl;
        return 1;
    }
}
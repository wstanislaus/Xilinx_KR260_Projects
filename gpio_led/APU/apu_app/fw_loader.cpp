#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <unistd.h>
#include <thread>
#include <chrono>
#include <cstring>
#include <getopt.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <algorithm>

using namespace std;

// --- Constants ---
const string RPU_BASE = "/sys/class/remoteproc/remoteproc0/";
const string PL_FIRMWARE_PATH = "/sys/class/fpga_manager/fpga0/firmware";
const string PL_FLAGS_PATH = "/sys/class/fpga_manager/fpga0/flags";
const string PL_STATE_PATH = "/sys/class/fpga_manager/fpga0/state";
const string DEFAULT_RPU_FW = "gpio_app.elf";
const string DEFAULT_PL_FW = "gpio_led.bit";

// --- Helpers ---

bool file_exists(const string& path) {
    struct stat buffer;
    return (stat(path.c_str(), &buffer) == 0);
}

string read_sysfs(const string& path, bool silent = false) {
    ifstream file(path);
    if (!file.is_open()) {
        if (!silent) cerr << "Error: Cannot open " << path << endl;
        return "";
    }
    string value;
    getline(file, value);
    return value;
}

bool write_sysfs(const string& path, const string& value) {
    ofstream file(path);
    if (!file.is_open()) {
        cerr << "Error: Cannot open " << path << " for writing (Check permissions)." << endl;
        return false;
    }
    file << value;
    return !file.fail();
}

// Converts Xilinx BIT file to BIN (strips header)
bool convert_bit_to_bin(const string& bit_path, const string& bin_path) {
    ifstream bit_file(bit_path, ios::binary | ios::ate);
    if (!bit_file.is_open()) return false;
    
    streamsize size = bit_file.tellg();
    bit_file.seekg(0, ios::beg);

    vector<unsigned char> buffer(size);
    if (!bit_file.read((char*)buffer.data(), size)) return false;

    // Strategy 1: Find Key 'e' (0x65) + 4-byte Length
    for (size_t i = 0; i < buffer.size() - 5; ++i) {
        if (buffer[i] == 0x65) {
            uint32_t len = (buffer[i+1] << 24) | (buffer[i+2] << 16) | (buffer[i+3] << 8) | buffer[i+4];
            if (i + 5 + len == buffer.size()) {
                ofstream bin_file(bin_path, ios::binary);
                bin_file.write((char*)&buffer[i+5], len);
                return true;
            }
        }
    }

    // Strategy 2: Sync Word (0xAA995566)
    const vector<unsigned char> sync_word = {0xAA, 0x99, 0x55, 0x66};
    auto sync_it = search(buffer.begin(), buffer.end(), sync_word.begin(), sync_word.end());
    if (sync_it == buffer.end()) return false;

    // Backtrack to include padding
    auto start_it = sync_it;
    while (start_it != buffer.begin() && *(start_it - 1) == 0xFF) --start_it;

    ofstream bin_file(bin_path, ios::binary);
    bin_file.write((char*)&(*start_it), distance(start_it, buffer.end()));
    return true;
}

// --- Loading Functions ---

void manage_rpu(bool start) {
    string action = start ? "start" : "stop";
    string current = read_sysfs(RPU_BASE + "state", true);
    
    if ((start && current == "running") || (!start && current == "offline")) {
        // Already in desired state
        return;
    }
    cout << (start ? "Starting" : "Stopping") << " RPU..." << endl;
    write_sysfs(RPU_BASE + "state", action);
}

void load_rpu(const string& fw_name) {
    if (fw_name.empty()) return;
    
    string fw_path = "/lib/firmware/" + fw_name;
    if (!file_exists(fw_path)) cerr << "Warning: " << fw_name << " not found in /lib/firmware/" << endl;

    manage_rpu(false); // Stop
    std::this_thread::sleep_for(std::chrono::milliseconds(200));
    
    cout << "Loading RPU Firmware: " << fw_name << endl;
    if (write_sysfs(RPU_BASE + "firmware", fw_name)) {
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        manage_rpu(true); // Start
    }
}

void load_pl(const string& fw_name) {
    if (fw_name.empty()) return;

    string final_name = fw_name;
    string fw_path = "/lib/firmware/" + fw_name;
    
    if (!file_exists(fw_path)) cerr << "Warning: " << fw_name << " not found in /lib/firmware/" << endl;

    // Handle .bit conversion
    if (fw_name.length() > 4 && fw_name.substr(fw_name.length() - 4) == ".bit") {
        string bin_name = fw_name.substr(0, fw_name.length() - 4) + ".bin";
        cout << "Converting " << fw_name << " to " << bin_name << "..." << endl;
        if (convert_bit_to_bin(fw_path, "/lib/firmware/" + bin_name)) {
            final_name = bin_name;
        } else {
            cerr << "Failed to convert .bit. Trying original." << endl;
        }
    }

    cout << "Loading PL Firmware: " << final_name << endl;
    write_sysfs(PL_FLAGS_PATH, "0"); // Full reconfiguration
    write_sysfs(PL_FIRMWARE_PATH, final_name);
    
    std::this_thread::sleep_for(std::chrono::milliseconds(500));
    string state = read_sysfs(PL_STATE_PATH);
    if (state != "operating") cerr << "Warning: PL State is " << state << endl;
    else cout << "PL Loaded Successfully." << endl;
}

void print_usage(const char* prog) {
    cout << "Usage: " << prog << " [firmware_files...]" << endl;
    cout << "  Auto-detects .bit/.bin (PL) and .elf (RPU)." << endl;
    cout << "  Defaults: " << DEFAULT_RPU_FW << ", " << DEFAULT_PL_FW << endl;
}

// --- Main ---

int main(int argc, char* argv[]) {
    if (geteuid() != 0) cerr << "Warning: Run as root." << endl;

    string rpu_fw = DEFAULT_RPU_FW;
    string pl_fw = DEFAULT_PL_FW;

    for (int i = 1; i < argc; i++) {
        string arg = argv[i];
        if (arg == "-h" || arg == "--help") {
            print_usage(argv[0]);
            return 0;
        }
        // Auto-detect based on extension
        if (arg.find(".bit") != string::npos || arg.find(".bin") != string::npos) {
            pl_fw = arg;
        } else if (arg.find(".elf") != string::npos) {
            rpu_fw = arg;
        }
    }
    
    load_pl(pl_fw);
    load_rpu(rpu_fw);

    return 0;
}

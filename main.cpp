#include <filesystem>
#include <stdio.h>
#include <unistd.h>
#include <iostream>
#include <fstream>
#include <string>
#include <json/json.h>
#include <vector>
#include <sys/stat.h>
#include <chrono>
#include <ctime>
#include <time.h>


std::filesystem::path RECORD_PATH; //sdrpp recording path
std::filesystem::path BB_PATH;     //baseband file recorded
int latest_time;

//Get the SDR config, and find the recording path
bool get_sdr_config() {
    const char* home = getenv("HOME");
    std::string cfg_path = std::string(home) + "/.config/sdrpp/recorder_config.json";

    std::cout << "Reading from " << cfg_path;
    std::ifstream cfg(cfg_path); 
    std::streamsize size = cfg.tellg();
    if (!cfg) { return false; }
    if (size > 10000) { return false; } //tooooo big

    Json::Value data;

    Json::CharReaderBuilder readerBuilder;

    std::string errs;

    Json::parseFromStream(readerBuilder, cfg, &data, &errs);

    RECORD_PATH = data["Recorder"]["recPath"].asString();
    std::cout << "\n Record path found at: " << RECORD_PATH << "\n";
    return true;
}

//Count amount of basebands
int count_files() {
    int count = 0;
    for (auto const & baseband : std::filesystem::recursive_directory_iterator(RECORD_PATH))
    {
        if (baseband.is_regular_file() && baseband.path().extension() == ".wav") {
            //filesystem has no cool way to get a unix type stamp
            count++;
        }
    }
    return count;
}

//Get latest baseband
std::filesystem::path get_latest_file() {

    std::filesystem::path latest_file;
    

    for (auto const & baseband : std::filesystem::directory_iterator(RECORD_PATH))
    {
        if (baseband.is_regular_file() && baseband.path().extension() == ".wav") {
            //filesystem has no cool way to get a unix type stamp
            auto ftime = std::filesystem::last_write_time(baseband);
            auto sctp = std::chrono::time_point_cast<std::chrono::system_clock::duration>(
            ftime - std::filesystem::file_time_type::clock::now() +
            std::chrono::system_clock::now());
            auto unix_time = std::chrono::duration_cast<std::chrono::seconds>(sctp.time_since_epoch()).count();

            if (unix_time > latest_time) {
                latest_file = baseband.path();
                latest_time = unix_time;
                //std::cout << unix_time << "\n";
            }
        }
    }
    return latest_file; //we got the latest file ! :3
}

std::stringstream get_timestamp() {
    std::string t_m, t_d; //UTC
    std::stringstream out;
    std::time_t liltime = static_cast<std::time_t>(latest_time);

    std::tm* silly = std::gmtime(&liltime);

    int month = silly->tm_mon;
    int day = silly->tm_mday;
    int hour = silly->tm_hour;
    int min = silly->tm_min;
    int sec = silly->tm_sec;
    
    
    switch (month) {
        case 0:
        t_m = "Jan"; break;
        case 1:
        t_m = "Feb"; break;
        case 2:
        t_m = "Mar"; break;
        case 3:
        t_m = "Apr"; break;
        case 4:
        t_m = "May"; break;
        case 5:
        t_m = "Jun"; break;
        case 6:
        t_m = "Jul"; break;
        case 7:
        t_m = "Aug"; break;
        case 8:
        t_m = "Sep"; break;
        case 9:
        t_m = "Oct"; break;
        case 10:
        t_m = "Nov"; break;
        case 11:
        t_m = "Dec"; break;
        
    }

    out <<     t_m << "-" << day << "-"
              << std::setw(2) << std::setfill('0') << hour  << "H"
              << std::setw(2) << std::setfill('0') << min   << "M"
              ;
    return out;

}

int main() {
    
    //Get record PATH
    if (!get_sdr_config()) printf("Could not get SDR++ recorder config!\n");

    int old_file_count = count_files();

    system("sdrpp");              //Run SDR++
    printf("SDR++ closed\n");

    int new_file_count = count_files();

    bool testing_override = false;
    if (testing_override) std::cout <<"Testing override on!!!!!!!!!!!!!!!!!!!!!!!!\n";

    //If no new files, stop
    if (new_file_count > old_file_count || testing_override) {BB_PATH = get_latest_file(); std::cout << "Baseband found at " << BB_PATH << "\n";} 
    else {std::cout << "No new files found! exiting.\n"; return -1;}

    std::string OUT_PATH = " " + RECORD_PATH.string() + "/"+ get_timestamp().str();
    //Start satdump
    std::stringstream command;
    command << "satdump meteor_m2-x_lrpt baseband " << BB_PATH << OUT_PATH << " fill_missing";

    std::cout << "Running: " << command.str() << "\n";
    system(command.str().c_str());

}

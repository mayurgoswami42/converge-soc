
#include "reader.hpp"
#include "utils/debug.hpp"
#include "utils/utils.hpp"

void Reader::reset_offset()
{
    DEBUG_PRINT("READER::INFO - Reseting offset!");
    offset = 0;
}

int Reader::get_offset() const
{
    return offset;
}

void Reader::close() const
{
    write_offset(offset);
}

// std::getline is not suitable for our case as the log files may be continuously getting written by server,
// std::getline returns and stop writing line when it hits std::fstream::eof() or '\n' character
// when server is writing the logs before server finish writing line std::getline reads till last character and due to eof hit, returns
// and we get part of line
// which is incorrect in our case, we need full log line to parse
// custom_getline returns when it hits '\n' character and hence give full line
bool Reader::custom_getline(std::ifstream &ifile, std::string &line)
{
    std::streampos before_pos = ifile.tellg();
    char c = ' ';
    std::string buffer;

    while (ifile.get(c))
    {
        if (c == '\n')
        {
            line = buffer;
            return true;
        }
        buffer += c;
    }

    ifile.clear();
    ifile.seekg(before_pos);
    return false;
}

// incase our socket stops, we need to continue from last we ended, hence we maintain offset of log file in storage/.reader_offset
std::streampos Reader::read_offset() const
{
    std::ifstream i_state(STATE_FILE);
    long long raw_offset = 0;
    if (i_state) i_state >> raw_offset;
    else
    {
        DEBUG_PRINT("READER::ERROR - Can't open reader state file!");
    }

    std::streampos offset = static_cast<std::streampos>(raw_offset);

    return offset;
}

// saves the current offset into the storage/.reader_offset
std::streampos Reader::write_offset(std::streampos offset) const
{
    std::ofstream o_state(STATE_FILE, std::ios::trunc);
    
    if (!o_state)
    {
        DEBUG_PRINT("READER::ERROR - Reader offset file misplaced!");
        return offset;
    }
    o_state << static_cast<long long>(offset);
    
    return offset;
}

void Reader::initialize_storage() const
{
    std::filesystem::create_directories("storage");

    // create new offset file if it doesn't already exists
    if (!std::filesystem::exists(STATE_FILE))
    {
        std::ofstream _file(STATE_FILE);
        _file.write("0", 1); // default offset
    }
}


Reader::Reader()
{
    initialize_storage();
    offset = read_offset();
    status = true;
}

std::vector<std::string> Reader::read_logs(std::string file_name)
{
    std::ifstream ifile(file_name);
    if (!ifile)
    {
        DEBUG_PRINT("READER::ERROR - Can't open log file!");
        // check the status of reader when you call read_logs
        status = false;
        return {};
    }

    if (ifile.peek() == std::ifstream::traits_type::eof())
    {
        DEBUG_PRINT("READER::INFO - Log file is empty!");
        utils::thread_sleep(1000);
        return {};
    }

    ifile.seekg(offset);

    std::vector<std::string> logs;
    std::string line{};

    while (custom_getline(ifile, line))
    {
        logs.emplace_back(line);
        offset = ifile.tellg(); // capturing offset in loop because we want offset to be last read line not -1 in case of eof
    }

    if (offset == -1) // ensure offset to be of last line not -1 (means eof), because log files updates continuously
    {
        ifile.clear();
        ifile.seekg(0, std::ios::end);
        offset = ifile.tellg();
    }

    return logs;
}
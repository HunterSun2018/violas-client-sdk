#include <iostream>
#include <string>
#include <fstream>
#include <map>
#include <functional>

using namespace std;

void run_test_libra(
    const string &url,    
    const string &mint_key_file,
    const string &mnemonic_file);

void run_test_token(const string &host,
                    uint16_t port,
                    const string &mnemonic_file,
                    const string &mint_key_file);

int main(int argc, char *argv[])
{
    ofstream file("log.txt");
    streambuf *mylog = clog.rdbuf(file.rdbuf());

    try
    {
        if (argc < 4)
        {
            cout << "usage : test_violas url mint_key_file mnemonic_file script_files_path";
            return -1;
        }

        using handler = function<void()>;
        map<int, handler> handlers = {
            {0, [=]() { run_test_libra(argv[1], argv[2], argv[3]); }},
            {1, [=]() { run_test_token(argv[1], stol(argv[2]), argv[3], argv[4]); }},
        };

        cout << "input index\n"
                "0 for testing Client, 1 for testing Token" << endl;
        int index ;
        cin >> index;

        handlers[index]();        
    }
    catch (const std::exception &e)
    {
        std::cerr << e.what() << '\n';
    }

    clog.rdbuf(mylog);

    return 0;
}
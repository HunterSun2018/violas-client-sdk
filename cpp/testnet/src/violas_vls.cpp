#include <iostream>
#include <map>
#include <string>
#include <sstream>
#include <tuple>
#include <functional>
#include <client.hpp>
#include "utils.h"
#include "argument.hpp"
#include "json.hpp"

using namespace std;
using namespace violas;

void initialize_timestamp(client_ptr client);
void mine_vls(client_ptr client);
void distribute_vls_to_all_service_admins(client_ptr client, bool is_paying_to_bank_admin);
void recover_vls_fees_to_association(client_ptr client);
void view_vls_info(client_ptr client);

const tuple<Address, string> VLS_ADDRESSES[] = {
    {{0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 'V', 'L', 'S', 0x00}, "VLS-TRASH"}, // 000000000000000000000000564C5300
    {{0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 'V', 'L', 'S', 0x01}, "VLS-COMM"},
    {{0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 'V', 'L', 'S', 0x02}, "VLS-ASSOCA"},
    {{0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 'V', 'L', 'S', 0x03}, "VLS-TEAM"},
    {{0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 'V', 'L', 'S', 0x04}, "VLS-ADVS"},
    {{0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 'V', 'L', 'S', 0x05}, "VLS-OPEN"},
};

int main(int argc, char *argv[])
{
    try
    {
        Arguments args;

        args.parse_command_line(argc, argv);
        args.show();

        auto client = Client::create(args.chain_id, args.url, args.mint_key, args.mnemonic, args.waypoint);

        client->test_connection();

        if (args.mnemonic.empty())
        {
            string mnemonic = "mnemonic/vls.mne";

            client->recover_wallet_accounts(mnemonic);
            cout << "Violas client is using mnemonic file "
                 << color::GREEN << mnemonic << color::RESET
                 << endl;
        }

        if (args.distrbuting)
        {
            distribute_vls_to_all_service_admins(client, true);
            return 0;
        }

        using handler = function<void()>;
        map<uint, handler> handlers = {
            {1, [=]() { initialize_timestamp(client); }},
            {2, [=]() {
                 string input;
                 cout << "Do you want to pay to bank admin? yes or no" << endl;
                 cin >> input;
                 bool is_paying_to_bank = input == "yes" ? true : false;
                 distribute_vls_to_all_service_admins(client, is_paying_to_bank);
             }},
            {3, [=]() { mine_vls(client); }},
            {4, [=]() { recover_vls_fees_to_association(client); }},
            {5, [=]() { view_vls_info(client); }},
        };

        uint index = 0;
        do
        {
            cout << "Function list\n"
                 << color::CYAN
                 << left << setw(10) << "Index"
                 << left << setw(50) << "Description"
                 << color::RESET
                 << endl;

            cout << left << setw(10) << "0" << left << setw(50) << "Quit" << endl
                 << left << setw(10) << "1" << left << setw(50) << "Initialize vls timestamp" << endl
                 << left << setw(10) << "2" << left << setw(50) << "Distribute vls to all Violas adminitrators" << endl
                 << left << setw(10) << "3" << left << setw(50) << "Mint VLS and distribute it to all receivers" << endl
                 << left << setw(10) << "4" << left << setw(50) << "Recover vls transaction fees to Violas association" << endl
                 << left << setw(10) << "5" << left << setw(50) << "View currency VLS information" << endl
                 << "Please input index : ";

            cin >> index;

            if (handlers.find(index) != end(handlers))
                handlers[index]();
        } while (index != 0);
    }
    catch (const std::exception &e)
    {
        std::cerr << color::RED
                  << "caught an exception : " << e.what()
                  << color::RESET
                  << endl;
    }

    return 0;
}

void initialize_timestamp(client_ptr client)
{
    client->allow_publishing_module(true);
    client->allow_custom_script();

    cout << "allow custom script and  publishing module." << endl;

    for (const auto address : VLS_ADDRESSES)
    {
        client->create_next_account(get<0>(address));
    }

    auto accounts = client->get_all_accounts();

    try_catch([&]() {
        for (auto account : accounts)
        {
            client->create_designated_dealer_ex("VLS", 0,
                                                account.address, account.auth_key,
                                                get<1>(VLS_ADDRESSES[account.index]),
                                                "wwww.violas.io",
                                                account.pub_key, true);

            cout << "address : " << account.address
                 << ", auth key : " << account.auth_key << endl;
        }

        cout << color::GREEN << "Created all accounts for VLS receivers." << color::RESET << endl;
    });

    vector<uint8_t> vls_initialize_timestamp = {
        161, 28, 235, 11, 1, 0, 0, 0, 5, 1, 0, 2, 3, 2, 5, 5, 7, 1, 7, 8, 25, 8, 33, 16, 0, 0, 0, 1, 0, 0, 0, 0,
        3, 86, 76, 83, 20, 105, 110, 105, 116, 105, 97, 108, 105, 122, 101, 95, 116, 105, 109, 101, 115, 116, 97,
        109, 112, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 2, 17, 0, 2};

    client->execute_script(0, vls_initialize_timestamp);

    cout << color::GREEN << "Initialized timestamp for VLS module." << color::RESET << endl;
}

void mine_vls(client_ptr client)
{
    vector<uint8_t> mint_vls_bytecode = {
        161, 28, 235, 11, 1, 0, 0, 0, 5, 1, 0, 2, 3, 2, 5, 5, 7, 1, 7, 8, 21, 8, 29, 16, 0, 0, 0, 1, 0, 0, 0, 0,
        11, 68, 105, 101, 109, 65, 99, 99, 111, 117, 110, 116, 8, 109, 105, 110, 101, 95, 118, 108, 115, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 2, 17, 0, 2};

    //string
    auto [address0, name] = VLS_ADDRESSES[0];
    client->create_next_account(address0);
    //cout << "account 0' address : " << address0 << endl;

    auto accounts = client->get_all_accounts();

    client->execute_script(0, mint_vls_bytecode);

    time_t rawtime = time(NULL);
    cout << color::GREEN
         << put_time(localtime(&rawtime), "%Y-%m-%d %X") << "succeeded to mine VLS."
         << color::RESET << endl;
}
//
//  distribute VLS to all  service admins
//
void distribute_vls_to_all_service_admins(client_ptr client, bool is_paying_to_bank_admin)
{
    static VecU8 script_bytecode = {
        161, 28, 235, 11, 1, 0, 0, 0, 8, 1, 0, 18, 2, 18, 16, 3, 34, 98, 4, 132, 1, 6, 5, 138, 1, 114, 7, 252, 1, 132, 3, 8, 128, 5, 16, 6, 144, 5, 75, 0, 0, 0, 1, 0, 2, 0, 3, 0, 4, 0, 5, 0, 6, 0, 7, 0, 8, 4, 4, 2, 0, 0, 21, 1, 0, 6, 27, 2, 0, 6, 6, 2, 0, 1, 9, 0, 1, 0, 2, 10, 1, 1, 0, 2, 11, 1, 1, 0, 3, 12, 2, 0, 0, 4, 13, 3, 4, 0, 4, 14, 5, 1, 0, 4, 15, 5, 1, 0, 5, 16, 6, 7, 0, 7, 17, 8, 9, 1, 1, 8, 18, 6, 10, 0, 8, 19, 11, 0, 0, 8, 20, 12, 0, 0, 0, 22, 7, 1, 1, 1, 0, 23, 6, 13, 0, 0, 24, 0, 0, 0, 0, 25, 14, 0, 1, 1, 0, 26, 13, 0, 0, 6, 28, 0, 15, 0, 6, 29, 16, 17, 0, 8, 16, 12, 20, 15, 20, 0, 1, 3, 4, 6, 12, 3, 3, 3, 2, 3, 3, 1, 8, 0, 2, 3, 8, 0, 1, 6, 12, 1, 5, 2, 6, 10, 9, 0, 3, 1, 6, 9, 0, 1, 1, 2, 6, 12, 10, 2, 2, 6, 12, 3, 1, 8, 1, 5, 6, 8, 1, 5, 3, 10, 2, 10, 2, 1, 10, 8, 2, 1, 8, 2, 2, 5, 8, 0, 2, 6, 12, 1, 25, 5, 8, 0, 3, 8, 0, 3, 3, 3, 3, 3, 8, 0, 8, 1, 8, 0, 6, 8, 2, 10, 8, 2, 5, 3, 7, 10, 8, 2, 3, 1, 3, 1, 3, 1, 3, 3, 1, 8, 3, 11, 68, 105, 101, 109, 65, 99, 99, 111, 117, 110, 116, 13, 68, 105, 101, 109, 84, 105, 109, 101, 115, 116, 97, 109, 112, 6, 69, 114, 114, 111, 114, 115, 8, 69, 120, 99, 104, 97, 110, 103, 101, 12, 70, 105, 120, 101, 100, 80, 111, 105, 110, 116, 51, 50, 6, 83, 105, 103, 110, 101, 114, 3, 86, 76, 83, 6, 86, 101, 99, 116, 111, 114, 10, 86, 105, 111, 108, 97, 115, 66, 97, 110, 107, 11, 110, 111, 119, 95, 115, 101, 99, 111, 110, 100, 115, 14, 108, 105, 109, 105, 116, 95, 101, 120, 99, 101, 101, 100, 101, 100, 16, 114, 101, 113, 117, 105, 114, 101, 115, 95, 97, 100, 100, 114, 101, 115, 115, 16, 115, 101, 116, 95, 110, 101, 120, 116, 95, 114, 101, 119, 97, 114, 100, 115, 20, 99, 114, 101, 97, 116, 101, 95, 102, 114, 111, 109, 95, 114, 97, 116, 105, 111, 110, 97, 108, 10, 100, 105, 118, 105, 100, 101, 95, 117, 54, 52, 12, 109, 117, 108, 116, 105, 112, 108, 121, 95, 117, 54, 52, 10, 97, 100, 100, 114, 101, 115, 115, 95, 111, 102, 6, 98, 111, 114, 114, 111, 119, 12, 105, 115, 95, 112, 117, 98, 108, 105, 115, 104, 101, 100, 7, 112, 117, 98, 108, 105, 115, 104, 18, 115, 101, 116, 95, 105, 110, 99, 101, 110, 116, 105, 118, 101, 95, 114, 97, 116, 101, 18, 87, 105, 116, 104, 100, 114, 97, 119, 67, 97, 112, 97, 98, 105, 108, 105, 116, 121, 7, 98, 97, 108, 97, 110, 99, 101, 27, 101, 120, 116, 114, 97, 99, 116, 95, 119, 105, 116, 104, 100, 114, 97, 119, 95, 99, 97, 112, 97, 98, 105, 108, 105, 116, 121, 8, 109, 105, 110, 101, 95, 118, 108, 115, 8, 112, 97, 121, 95, 102, 114, 111, 109, 27, 114, 101, 115, 116, 111, 114, 101, 95, 119, 105, 116, 104, 100, 114, 97, 119, 95, 99, 97, 112, 97, 98, 105, 108, 105, 116, 121, 8, 82, 101, 99, 101, 105, 118, 101, 114, 13, 103, 101, 116, 95, 114, 101, 99, 101, 105, 118, 101, 114, 115, 15, 117, 110, 112, 97, 99, 107, 95, 114, 101, 99, 101, 105, 118, 101, 114, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 5, 16, 88, 92, 106, 163, 29, 251, 25, 196, 175, 32, 232, 225, 65, 18, 203, 63, 3, 8, 233, 3, 0, 0, 0, 0, 0, 0, 3, 8, 234, 3, 0, 0, 0, 0, 0, 0, 3, 8, 232, 3, 0, 0, 0, 0, 0, 0, 10, 2, 2, 1, 0, 10, 2, 1, 0, 5, 16, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 86, 76, 83, 0, 0, 18, 19, 147, 1, 10, 0, 17, 7, 12, 16, 17, 14, 17, 17, 12, 15, 13, 15, 12, 18, 11, 18, 46, 6, 0, 0, 0, 0, 0, 0, 0, 0, 56, 0, 12, 14, 11, 14, 20, 17, 18, 12, 13, 12, 2, 10, 16, 10, 2, 33, 7, 3, 17, 2, 12, 21, 12, 20, 11, 20, 3, 31, 11, 0, 1, 11, 21, 39, 10, 16, 56, 1, 12, 4, 10, 4, 11, 13, 17, 5, 12, 26, 6, 16, 0, 0, 0, 0, 0, 0, 0, 6, 100, 0, 0, 0, 0, 0, 0, 0, 17, 4, 12, 5, 6, 30, 0, 0, 0, 0, 0, 0, 0, 6, 100, 0, 0, 0, 0, 0, 0, 0, 17, 4, 12, 11, 6, 15, 0, 0, 0, 0, 0, 0, 0, 6, 100, 0, 0, 0, 0, 0, 0, 0, 17, 4, 12, 3, 10, 1, 3, 53, 5, 88, 10, 0, 17, 9, 9, 33, 3, 59, 5, 62, 10, 0, 7, 4, 17, 10, 10, 26, 11, 5, 17, 6, 12, 6, 10, 16, 56, 1, 12, 4, 10, 0, 10, 6, 17, 11, 10, 16, 56, 1, 10, 4, 10, 6, 23, 33, 7, 1, 17, 1, 12, 23, 12, 22, 11, 22, 3, 88, 11, 0, 1, 11, 23, 39, 10, 26, 11, 11, 17, 6, 12, 7, 17, 0, 12, 17, 10, 17, 6, 128, 81, 1, 0, 0, 0, 0, 0, 22, 12, 10, 10, 16, 56, 1, 12, 4, 10, 0, 10, 7, 10, 17, 10, 10, 17, 3, 10, 16, 56, 1, 10, 4, 10, 7, 23, 33, 7, 2, 17, 1, 12, 25, 12, 24, 11, 24, 3, 122, 11, 0, 1, 11, 25, 39, 11, 0, 17, 13, 12, 12, 10, 26, 11, 3, 17, 6, 12, 8, 14, 12, 7, 0, 10, 8, 7, 5, 7, 5, 56, 2, 10, 16, 56, 1, 12, 9, 14, 12, 7, 6, 10, 9, 7, 5, 7, 5, 56, 2, 11, 12, 17, 16, 2};

    for (auto [address, name] : VLS_ADDRESSES)
    {
        client->create_next_account(address);
    }

    auto accounts = client->get_all_accounts();

    // auto vls_balance = client->get_currency_balance(accounts[1].address, "VLS");
    // cout << "account 1's VLS balance : " << (double)vls_balance / MICRO_COIN << endl;
    
    client->execute_script(1, script_bytecode, {}, {is_paying_to_bank_admin});

    time_t rawtime = time(NULL);
    cout << color::GREEN
         << put_time(localtime(&rawtime), "%Y-%m-%d %X")
         << " - distribute VLS from VLS-COMM to all service adminitrators"
         << color::RESET << endl;
}

void recover_vls_fees_to_association(client_ptr client)
{
    static VecU8 script_bytecode = {
        161, 28, 235, 11, 1, 0, 0, 0, 5, 1, 0, 2, 3, 2, 5, 5, 7, 5, 7, 12, 44, 8, 56, 16, 0, 0, 0, 1, 0, 1, 0, 2, 6, 12, 5, 0, 11, 68, 105, 101, 109, 65, 99, 99, 111, 117, 110, 116, 31, 114, 101, 99, 111, 118, 101, 114, 95, 118, 108, 115, 95, 102, 101, 101, 115, 95, 116, 111, 95, 97, 115, 115, 111, 99, 105, 97, 116, 105, 111, 110, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 1, 4, 11, 0, 10, 1, 17, 0, 2};

    for (auto [address, name] : VLS_ADDRESSES)
    {
        client->create_next_account(address);
    }

    auto accounts = client->get_all_accounts();
    const size_t ASSOCIATION_INDEX_ID = 2;
    const auto &association_address = accounts[ASSOCIATION_INDEX_ID].address;

    auto vls_balance = client->get_currency_balance(association_address, "VLS");
    cout << "Association account's VLS balance : " << vls_balance << endl;

    client->execute_script(VIOLAS_TREASURY_COMPLIANCE_ACCOUNT_ID, script_bytecode, {}, {association_address});

    vls_balance = client->get_currency_balance(accounts[ASSOCIATION_INDEX_ID].address, "VLS");
    cout << "Association account's VLS balance : " << vls_balance << endl;

    //cout << format("Association account's VLS balance : %d", vls_balance) << endl;
}

void view_vls_info(client_ptr client)
{
    using json = nlohmann::json;

    cout << "all currency info : " << endl;

    cout << color::CYAN
         << left << setw(20) << "Code"
         << left << setw(20) << "Total/scaling"
         << left << setw(20) << "Total value"
         << left << setw(20) << "Preburn value"
         << left << setw(20) << "Scaling factor"
         << color::RESET << endl;

    auto currencies = json::parse(client->get_all_currency_info());

    for (auto &currency : currencies)
    {
        //{"code":"VLS","fractional_part":1000,"preburn_value":0,"scaling_factor":1000000,"to_lbr_exchange_rate":1.0,"total_value":2136950000000}
        auto total_div_scaling = uint64_t(currency["total_value"]) / uint64_t(currency["scaling_factor"]);
        auto row_color = currency["code"] == "VLS" ? color::GREEN : color::YELLOW;

        cout << row_color
             << left << setw(20) << string(currency["code"])
             << left << setw(20) << total_div_scaling
             << left << setw(20) << currency["total_value"].dump()
             << left << setw(20) << currency["preburn_value"].dump()
             << left << setw(20) << currency["scaling_factor"].dump()
             << color::RESET << endl;
    }

    cout << "VLS Receiver info :" << endl;
    cout << color::CYAN
         << left << setw(20) << "Name"
         << left << setw(40) << "Address"
         << left << setw(20) << "VLS balance"
         << color::RESET << endl;

    for (auto &[address, name] : VLS_ADDRESSES)
    {
        ostringstream oss;
        oss << address;

        cout << color::GREEN
             << left << setw(20) << name
             << left << setw(40) << oss.str()
             << left << setw(20) << client->get_currency_balance(address, "VLS") / (double)1'000'000
             << color::RESET << endl;
    }
    //
    //
    //
    tuple<Address, string> DEFI_admins[] =
        {
            {{0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0x42, 0x41, 0x4E, 0x4B}, "Bank DD admin"},
            {{0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0x45, 0x58, 0x43, 0x48}, "Exchange DD amdin"},
            {{0x58, 0x5c, 0x6a, 0xa3, 0x1d, 0xfb, 0x19, 0xc4, 0xaf, 0x20, 0xe8, 0xe1, 0x41, 0x12, 0xcb, 0x3f}, "Backend DD admin"},
        };

    cout << "Violas DEFI administrator accounts info" << endl;
    cout << color::CYAN
         << left << setw(20) << "Name"
         << left << setw(40) << "Address"
         << left << setw(20) << "VLS balance"
         << color::RESET << endl;

    for (auto &[address, name] : DEFI_admins)
    {
        ostringstream oss;
        oss << address;

        cout << color::GREEN
             << left << setw(20) << name
             << left << setw(40) << oss.str()
             << left << setw(20) << client->get_currency_balance(address, "VLS") / (double)1'000'000
             << color::RESET << endl;
    }
}
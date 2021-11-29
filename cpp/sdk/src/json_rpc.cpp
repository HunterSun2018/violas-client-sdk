#include <cpprest/filestream.h>
#include <cpprest/http_client.h>
#include <cpprest/json.h>
#include "../include/utils.hpp"
#include "../include/json_rpc.hpp"

using namespace std;
using namespace utility;
using namespace web;
using namespace web::http;
using namespace web::http::client;

namespace json_rpc
{

    class ClientImp : public Client
    {
        http_client cli;

    public:
        ClientImp(string_view url) : cli(U(string(url)), client_config_for_proxy())
        {
        }

        virtual ~ClientImp()
        {
        }

        web::http::client::http_client_config client_config_for_proxy()
        {
            web::http::client::http_client_config client_config;
#ifdef _WIN32
            wchar_t *pValue = nullptr;
            std::unique_ptr<wchar_t, void (*)(wchar_t *)> holder(nullptr, [](wchar_t *p)
                                                                 { free(p); });
            size_t len = 0;
            auto err = _wdupenv_s(&pValue, &len, L"http_proxy");
            if (pValue)
                holder.reset(pValue);
            if (!err && pValue && len)
            {
                std::wstring env_http_proxy_string(pValue, len - 1);
#else
            if (const char *env_http_proxy = std::getenv("http_proxy"))
            {
                std::string env_http_proxy_string(env_http_proxy);
#endif
                if (env_http_proxy_string == U("auto"))
                    client_config.set_proxy(web::web_proxy::use_auto_discovery);
                else
                    client_config.set_proxy(web::web_proxy(env_http_proxy_string));
            }

            return client_config;
        }

        virtual void submit(const diem_types::SignedTransaction &signed_txn) override
        {
            auto data = bytes_to_hex(signed_txn.bcsSerialize());

            string method = format(R"({"jsonrpc":"2.0","method":"submit","params":["%s"],"id":1})", data.c_str());
            string content_type = "application/json";

            auto rpc_response = cli.request(methods::POST, "/", method, content_type)
                                    .then([=](http_response response) -> pplx::task<json::value>
                                          {
                                              if (response.status_code() != 200)
                                                  __throw_runtime_error(response.extract_string().get().c_str());

                                              return response.extract_json(); })
                                    .get();

            auto error = rpc_response["error"];
            if (!error.is_null())
                __throw_runtime_error(("fun : get_account_state_blob, error : " + error.serialize()).c_str());

            auto version = rpc_response["diem_ledger_version"].as_integer();
        }

        virtual AccountView
        get_account(const diem_types::AccountAddress address, std::optional<uint64_t> version) override
        {
            string method = format(R"({"jsonrpc":"2.0","method":"get_account","params":["%s"],"id":1})", bytes_to_hex(address.value).c_str());
            string content_type = "application/json";

            auto rpc_response = cli.request(methods::POST, "/", method, content_type)
                                    .then([=](http_response response) -> pplx::task<json::value>
                                          {                                              

                                              if (response.status_code() != 200)
                                                  __throw_runtime_error(response.extract_string().get().c_str());

                                              return response.extract_json(); })
                                    .get();

            auto error = rpc_response["error"];
            if (!error.is_null())
                __throw_runtime_error(("fun : get_account_state_blob, error : " + error.serialize()).c_str());

            auto result = rpc_response["result"];

            AccountView view;

            if (!result["sequence_number"].is_null())
            {
                view.sequence_number = result["sequence_number"].as_number().to_int64();
            }
            return view;
        }

        virtual std::vector<Currency> get_currencies() override
        {
            vector<Currency> crc;

            return crc;
        }

        virtual AccountStateWithProof
        get_account_state_blob(string account_address) override
        {
            AccountStateWithProof asp;
            string method = format(R"({"jsonrpc":"2.0","method":"get_account_state_with_proof","params":["%s", null, null],"id":1})", account_address.c_str());
            string content_type = "application/json";

            auto rpc_response = cli.request(methods::POST, "/", method, content_type)
                                    .then([=](http_response response) -> pplx::task<json::value>
                                          {
                                              // printf("Response status code %u returned.\n", response.status_code());

                                              if (response.status_code() != 200)
                                                  __throw_runtime_error(response.extract_string().get().c_str());

                                              return response.extract_json(); })
                                    .get();

            auto error = rpc_response["error"];
            if (!error.is_null())
                __throw_runtime_error(("fun : get_account_state_blob, error : " + error.serialize()).c_str());

            auto result = rpc_response["result"];
            // cout << result.serialize() << endl;

            asp.version = result["version"].as_integer();

            auto blob = result["blob"];
            // if (blob.is_null())
            //     __throw_runtime_error(("fun : get_account_state_blob, error : " + rpc_response.serialize()).c_str());

            if (!blob.is_null())
                asp.blob = blob.as_string();

            asp.proof.ledger_info_to_transaction_info_proof = result["proof"]["ledger_info_to_transaction_info_proof"].as_string();
            asp.proof.transaction_info = result["proof"]["transaction_info"].as_string();
            asp.proof.transaction_info_to_account_proof = result["proof"]["transaction_info_to_account_proof"].as_string();

            return asp;
        }

        virtual std::vector<EventView>
        get_events(std::string event_key, uint64_t start, uint64_t limit, uint64_t rpc_id) override
        {
            vector<EventView> events;
            string method = format(R"({"jsonrpc":"2.0","method":"get_events","params":["%s", %d, %d],"id":1})",
                                   event_key.c_str(),
                                   start,
                                   limit,
                                   rpc_id);
            string content_type = "application/json";

            auto rpc_response = cli.request(methods::POST, "/", method, content_type)
                                    .then([=](http_response response) -> pplx::task<json::value>
                                          {
                                              if (response.status_code() != 200)
                                                  __throw_runtime_error(response.extract_string().get().c_str());

                                              return response.extract_json(); })
                                    .get();

            auto error = rpc_response["error"];
            if (!error.is_null())
                __throw_runtime_error(("fun : get_events, error : " + error.serialize()).c_str());

            auto result = rpc_response["result"];
            for (auto &e : result.as_array())
            {
                // cout << e.serialize() << endl;

                EventView ev;

                ev.key = e["key"].as_string();
                ev.sequence_number = e["sequence_number"].as_integer();
                ev.transaction_version = e["transaction_version"].as_integer();

                if (e["data"]["type"].as_string() == "unknown")
                {
                    UnknownEvent ue;

                    ue.bytes = hex_to_bytes(e["data"]["bytes"].as_string());

                    ev.event = ue;
                }

                events.emplace_back(ev);
            }

            return events;
        }
    };

    std::shared_ptr<Client>
    Client::create(std::string_view url)
    {
        return make_shared<ClientImp>(url);
    }

}

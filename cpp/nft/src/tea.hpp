#ifndef TEA_HPP
#define TEA_HPP
//#include ""

using TokenId = std::array<uint8_t, 32>;
struct Tea;

TokenId compute_token_id(const Tea &t);

struct Tea
{
    uint8_t kind;
    std::vector<uint8_t> manufacture;
    std::vector<uint8_t> PA; // Production Area
    uint64_t PD;             // Production Date
    std::vector<uint8_t> SN; // Sequence Number

    BcsSerde &serde(BcsSerde &bs)
    {
        return bs && kind && manufacture && PA && PD && SN;
    }
};

std::ostream &operator<<(std::ostream &os, const Tea &tea)
{
    std::cout << "Tea { "
              << "Kind : " << short(tea.kind) << ", "
              << "manufacture : " << std::string(begin(tea.manufacture), end(tea.manufacture)) << ", "
              << "Production Arem : " << std::string(begin(tea.PA), end(tea.PA)) << ", "
              << "Production Date : " << tea.PD << ", "
              << "SN : " << std::string(tea.SN.begin(), tea.SN.end()) 
              << " }, "
              << "token id : " << compute_token_id(tea);

    return os;
}

struct NftInfo
{
    bool limited;
    uint64_t total;
    uint64_t amount;
    violas::Address admin;
    std::map<std::vector<uint8_t>, std::vector<violas::Address>> owners;
    violas::EventHandle mint_event;
    violas::EventHandle burn_event;

    BcsSerde &serde(BcsSerde &bs)
    {
        return bs && limited && total && amount && admin && owners && mint_event && burn_event;
    }
};

std::ostream &operator<<(std::ostream &os, const NftInfo &tea)
{
    // std::string identity(tea.identity.begin(), tea.identity.end());
    // std::string manufacture(begin(tea.manufacture), end(tea.manufacture));

    // std::cout << "Tea { "
    //           << "Identity : " << identity << ", "
    //           << "Kind : " << short(tea.kind) << ", "
    //           << "manufacture : " << manufacture << ", "
    //           << "Date : " << tea.date
    //           << " }, "
    //           << "token id : " << compute_token_id(tea);

    return os;
}

struct NftTea
{
    std::vector<Tea> teas;
    violas::EventHandle sent_event;
    violas::EventHandle received_event;

    BcsSerde &serde(BcsSerde &bs)
    {
        return bs && teas && sent_event && received_event;
    }
};

inline std::ostream &operator<<(std::ostream &os, const NftTea &tea_nft)
{
    for (const auto &t : tea_nft.teas)
    {
        std::cout << t << std::endl;
    }

    return os;
}

struct ReceivedEvent
{
    std::vector<uint8_t> token_id;
    violas::Address payer;
    std::vector<uint8_t> metadata;

    BcsSerde &serde(BcsSerde &bs)
    {
        return bs && token_id && payer && metadata;
    }
};

#endif

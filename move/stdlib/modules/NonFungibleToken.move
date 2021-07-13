address 0x2 {
// 
// key is the token_id(:vector<u8>), stored in a sorted linked list
// value is a struct 'NonFungibleToken', contains the non fungible token
// the account address of each list node is actually the owner of the token
module NonFungibleToken {
    use 0x1::BCS;
    use 0x1::Compare;
    use 0x1::Errors;
    use 0x1::Event::{Self, EventHandle};
    use 0x1::Hash;    
    use 0x1::Option::{Self, Option};
    use 0x1::Signer;
    use 0x1::Vector;
    use 0x2::Map::{Self, Map};

    const NFT_PUBLISHER: address = 0xA550C18;   // Diem root account
    const EPAYEE_CANT_ACCEPT_NFT_TYPE: u64 = 1001;
    const ESENDER_HAS_ACCEPTED_NFT_TYPE: u64 = 1002;
    const ENFT_TOKEN_HAS_ALREADY_EXISTED: u64 = 1003;
    const ENFT_TOKEN_HAS_NOT_EXISTED: u64 = 1004;    
    
    struct MintEvent has drop, store {
        token_id: vector<u8>,
        receiver: address,
    }

    struct BurnEvent has drop, store {
        token_id: vector<u8>,
    }

    struct SentEvent has drop, store {
        token_id: vector<u8>,
        payee: address,
        metadata: vector<u8>,
    }

    struct ReceivedEvent has drop, store {
        token_id: vector<u8>,
        payer: address,
        metadata: vector<u8>,
    }

    struct Info<Token> has key, store {
        limited: bool,
        total: u64,
        amount: u64,
        admin: address, // has minting and burning permission
        owners: Map<vector<u8>, vector<address>>,  // token id maps to owner's address
        mint_events: EventHandle<MintEvent>,
        burn_events: EventHandle<BurnEvent>,
    }

    struct NonFungibleToken<Token> has key, store {
        tokens: vector<Token>,                          // store all tokens that has different token id
        sent_events: EventHandle<SentEvent>,            // sent token events
        received_events: EventHandle<ReceivedEvent>,    // received token events
    }

    struct TokenLock<Token> has key {
    }    

    fun lock<Token: store>(account: &signer) {
        move_to<TokenLock<Token>>(account, TokenLock<Token>{});
    }

    fun unlock<Token: store>(account: &signer) acquires TokenLock {
        let sender = Signer::address_of(account);
        let TokenLock<Token> {} = move_from<TokenLock<Token>>(sender);
    }
    //
    //
    //
    fun increment_nft_amount<Token: store>() 
    acquires Info {
        let info = borrow_global_mut<Info<Token>>(NFT_PUBLISHER);
        
        * (&mut info.amount) = info.amount + 1;
        
        assert(info.amount <= info.total, 1000);
    }
    //
    //  check if the address of admin 
    //
    fun check_admin_permission<Token: store>(admin: address)
    acquires Info {
        let info = borrow_global<Info<Token>>(NFT_PUBLISHER);

        assert(info.admin == admin, 10001);
    }

    public fun register<Token: store>(sig: &signer, limited: bool, total: u64, admin: address) {
        let sender = Signer::address_of(sig);
        assert(sender == NFT_PUBLISHER, 8000);

        let info = Info<Token> {
            limited: limited,
            total: total,
            amount: 0,
            admin,
            owners:  Map::empty<vector<u8>, vector<address>>(),
            mint_events: Event::new_event_handle<MintEvent>(sig),
            burn_events: Event::new_event_handle<BurnEvent>(sig)
        };

        move_to<Info<Token>>(sig, info);        
    }    
    //
    //  Get the number of balance for Token
    //
    public fun balance<Token : key + store>(sig: &signer) : u64 
    acquires NonFungibleToken {
        let sender = Signer::address_of(sig);

        let nft = borrow_global<NonFungibleToken<Token>>(sender);
        
        Vector::length<Token>(&nft.tokens)
    }
    //
    //
    //
    public fun make_token_id<Token>(token: &Token) : vector<u8> {
        let token_bcs = BCS::to_bytes<Token>(token);
        let token_id = Hash::sha3_256(token_bcs);

        token_id
    }
    //
    //  Get NFT token index by token id
    //  if returned index is equal to the length of vector that means faild to get index
    fun get_token_index<Token: key+store>(tokens: &vector<Token>, token_id: &vector<u8>) : u64 {

        let length = Vector::length<Token>(tokens);
        let i: u64 = 0;
        
        while(i < length) {
            let token = Vector::borrow<Token>(tokens, i);
            if( Compare::cmp_bcs_bytes(&make_token_id(token), token_id) == 0 )
            {
                break
            };
        };

        i
    }
    //
    //  Get NFT token from an account
    //
    fun get_nft_token<Token: key+store>(account: &signer, token_id: &vector<u8>): Token 
    acquires NonFungibleToken {
        let sender = Signer::address_of(account);
        assert(exists<NonFungibleToken<Token>>(sender), 8006);
        assert(!exists<TokenLock<Token>>(sender), 8007);
        Self::lock<Token>(account);

        let nft = borrow_global_mut<NonFungibleToken<Token>>(sender);    
        let length = Vector::length<Token>(&nft.tokens);

        let index = get_token_index<Token>(&nft.tokens, token_id);
        assert(index < length, 10001);

        if (index != length-1)
            Vector::swap_remove<Token>(&mut nft.tokens, index)
        else
            Vector::pop_back<Token>(&mut nft.tokens)        
    }   
    //
    //  get token owner by token id
    //
    public fun owner<Token:key+store>(token_id: &vector<u8>): Option<address>
    acquires Info {
        let info = borrow_global<Info<Token>>(NFT_PUBLISHER);

        let (index, found) = Map::find<vector<u8>, vector<address>>(&info.owners, token_id);
        if( found ) {
            let(_, owners) = Map::borrow(&info.owners, index);            
            let length = Vector::length(owners);
        
            Option::some(*Vector::borrow(owners, length-1))
        } else {
            Option::none()
        }
        
    }
    ///
    /// Mint a NFT to a receiver
    /// 
    public fun mint<Token: copy + drop + store>(sig: &signer, receiver: address, token: Token) : bool
    acquires NonFungibleToken, Info  {

        let sender = Signer::address_of(sig);

        check_admin_permission<Token>(sender);

        // The receiver must has called method 'accept_token' previously
        assert(exists<NonFungibleToken<Token>>(receiver), Errors::not_published(EPAYEE_CANT_ACCEPT_NFT_TYPE));
        
        let token_id = make_token_id<Token>(&token);                

        let info = borrow_global_mut<Info<Token>>(NFT_PUBLISHER);
        
        // Insert to global map
        let ret = Map::insert(&mut info.owners, copy token_id, Vector::singleton(receiver));
        
        // Abort if token id has already existed            
        assert( ret, Errors::invalid_argument(ENFT_TOKEN_HAS_ALREADY_EXISTED) );  
        
        // Emit sent event
        Event::emit_event(&mut info.mint_events, MintEvent{ token_id, receiver });
        
        // Increment NFT amount
        increment_nft_amount<Token>();
        //
        //  Deposite NFT to receiver        
        //
        let receiver_token_ref_mut = borrow_global_mut<NonFungibleToken<Token>>(receiver);    
                
        Vector::push_back<Token>(&mut receiver_token_ref_mut.tokens, token);        
        
        true
    }     
    //
    //  Burn a NFT token
    //
    public fun burn<Token: copy+drop+key+store>(sig: &signer, token_id: &vector<u8>)
    acquires NonFungibleToken, Info  {
        let sender = Signer::address_of(sig);
                
        check_admin_permission<Token>(sender);

        // Erase all owners by token id
        let info = borrow_global_mut<Info<Token>>(NFT_PUBLISHER);
        let ret = Map::erase<vector<u8>, vector<address>>(&mut info.owners, token_id);
        assert(ret, Errors::invalid_argument(ENFT_TOKEN_HAS_NOT_EXISTED));
        
        // drop token by token id
        let _token = get_nft_token<Token>(sig, token_id);

        // Emit sent event        
        Event::emit_event(&mut info.burn_events, BurnEvent{ token_id: *token_id });
    }
    //
    //  Accept NFT tokens
    //
    public fun accept<Token: store>(sig: &signer) {
        let sender = Signer::address_of(sig);
                
        assert(!exists<NonFungibleToken<Token>>(sender), Errors::already_published(ESENDER_HAS_ACCEPTED_NFT_TYPE));
        
        move_to<NonFungibleToken<Token>>(sig, 
            NonFungibleToken<Token> {
                tokens: Vector::empty<Token>(),
                sent_events: Event::new_event_handle<SentEvent>(sig),
                received_events: Event::new_event_handle<ReceivedEvent>(sig),
                });
    }
    //
    //  Transfer a NFT token with token id
    //
    public fun transfer<Token: drop + key + store>(sig: &signer, receiver: address, token_id: &vector<u8>, metadata: vector<u8>) 
    acquires NonFungibleToken, Info {
        let sender = Signer::address_of(sig);
        assert(sender != receiver, 10010);

        let sender_token_ref_mut = borrow_global_mut<NonFungibleToken<Token>>(sender);
        let index = get_token_index(&sender_token_ref_mut.tokens, token_id);
        
        transfer_via_index<Token>(sig, receiver, index, metadata);
    }
    //
    //  Transfer a NFT token with index
    //
    public fun transfer_via_index<Token: drop + store>(account: &signer, receiver: address, index: u64, metadata: vector<u8>) 
    acquires NonFungibleToken, Info {
        let sender = Signer::address_of(account);

        assert(exists<NonFungibleToken<Token>>(receiver), 8002);                                
        assert(!exists<TokenLock<Token>>(sender), 8004);

        let sender_nft = borrow_global_mut<NonFungibleToken<Token>>(sender);
        let length = Vector::length<Token>(&sender_nft.tokens);
        
        // Ensure the index is valid 
        assert(index < length, 8003);
        
        // Get token from sender
        if (index < length - 1) //swap element to back
            Vector::swap<Token>(&mut sender_nft.tokens, index, length-1); 

        let token = Vector::pop_back(&mut sender_nft.tokens);
        let token_id = make_token_id(&token);
        
        // Update owner of token id
        let info = borrow_global_mut<Info<Token>>(NFT_PUBLISHER);
        
        let (index, found) = Map::find(&info.owners, &token_id);
        if(found) {
            let (_, value) = Map::borrow_mut(&mut info.owners, index);
            Vector::push_back(value, receiver);
        } else {
            abort(8004)
        };       
        
        // Emit sent event
        Event::emit_event(&mut sender_nft.sent_events, SentEvent{ token_id: copy token_id, payee: receiver, metadata: copy metadata });

        // Put token to receiver
        let nft_receiver =  borrow_global_mut<NonFungibleToken<Token>>(receiver);
        Vector::push_back<Token>(&mut nft_receiver.tokens, token);

        // Emit transfer event
        Event::emit_event(&mut nft_receiver.received_events, ReceivedEvent{ token_id, payer: sender, metadata });
    }
}
}

/*
//! new-transaction
//! sender: nftservice
module TestNft {
    struct TestNft {}
    public fun new_test_nft(): TestNft {
        TestNft{}
    }
}
// check: "Keep(EXECUTED)"

//! new-transaction
//! sender: alice
// sample for moving Nft into another resource
module MoveNft {
    use {{nftservice}}::NonFungibleToken::{Self, NonFungibleToken};
    use {{nftservice}}::TestNft::TestNft;
    use 0x1::Signer;

    resource struct MoveNft {
        nft: NonFungibleToken<TestNft>
    }

    public fun move_nft(account: &signer) {
        let nft = NonFungibleToken::get_nft<TestNft>(account);
        move_to<MoveNft>(account, MoveNft{ nft });
    }

    public fun move_back_nft(account: &signer) acquires MoveNft {
        let sender = Signer::address_of(account);
        let MoveNft { nft } = move_from<MoveNft>(sender);
        NonFungibleToken::put_nft<TestNft>(account, nft);
    }
}
// check: "Keep(EXECUTED)"

//! new-transaction
//! sender: nftservice
script {
use {{nftservice}}::NonFungibleToken;
use {{nftservice}}::TestNft::TestNft;
fun main(account: signer) {
    NonFungibleToken::initialize<TestNft>(account, false, 0);
}
}

// check: "Keep(EXECUTED)"

//! new-transaction
//! sender: alice
script {
use {{nftservice}}::NonFungibleToken;
use {{nftservice}}::TestNft::{Self, TestNft};
use 0x1::Hash;
fun main(account: signer) {
    let input = b"input";
    let token_id = Hash::sha2_256(input);
    let token = TestNft::new_test_nft();
    NonFungibleToken::preemptive<TestNft>(account, {{nftservice}}, token_id, token);
}
}

// check: "Keep(EXECUTED)"

//! new-transaction
//! sender: alice
script {
use {{alice}}::MoveNft;
fun main(account: signer) {
    MoveNft::move_nft(account);
}
}

// check: "Keep(EXECUTED)"

//! new-transaction
//! sender: bob
script {
use {{nftservice}}::NonFungibleToken;
use {{nftservice}}::TestNft::TestNft;
fun main(account: signer) {
    NonFungibleToken::accept_token<TestNft>(account);
}
}

// check: "Keep(EXECUTED)"

//! new-transaction
//! sender: alice
script {
use {{nftservice}}::NonFungibleToken;
use {{nftservice}}::TestNft::TestNft;
use 0x1::Hash;
fun main(account: signer) {
    let input = b"input";
    let token_id = Hash::sha2_256(input);
    NonFungibleToken::safe_transfer<TestNft>(account, {{nftservice}}, token_id, {{bob}});
}
}

// check: ABORTED

//! new-transaction
//! sender: alice
script {
use {{alice}}::MoveNft;
fun main(account: signer) {
    MoveNft::move_back_nft(account);
}
}

// check: "Keep(EXECUTED)"

//! new-transaction
//! sender: alice
script {
use {{nftservice}}::NonFungibleToken;
use {{nftservice}}::TestNft::TestNft;
use 0x1::Hash;
fun main(account: signer) {
    let input = b"input";
    let token_id = Hash::sha2_256(input);
    NonFungibleToken::safe_transfer<TestNft>(account, {{nftservice}}, token_id, {{bob}});
}
}

// check: "Keep(EXECUTED)"
*/
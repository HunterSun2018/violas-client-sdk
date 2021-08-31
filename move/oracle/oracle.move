address 0x1 {

module Oracle {

use DiemFramework::Diem::{Self};
use DiemFramework::DiemTimestamp;
use Std::Signer;
//use DiemFramework::CoreAddresses;
use Std::FixedPoint32::{Self, FixedPoint32};
use Std::Event::{ Self, EventHandle };

    /// Evnet for updating
    struct UpdateEvent has store, drop {
        value : FixedPoint32,
        timestamp : u64,
        currency_code : vector<u8>        
    }

    /// Exchange rate for a currency / USD
    struct ExchangeRate<CoinType: store> has store, key { 
        value : FixedPoint32,
        timestamp : u64,    //Unix time in microseconds
        /// Event stream for updating exchange rate and where `update_exchange_rate`s will be emitted.
        update_events: EventHandle<UpdateEvent>
    }

    const EINVALID_SINGLETON_ADDRESS: u64 = 0;
    const ENOT_A_REGISTERED_CURRENCY: u64 = 1;

    fun emit_updating_events<CoinType: store>( exchange_rate: &mut ExchangeRate<CoinType>)
    {
        Event::emit_event(
                &mut exchange_rate.update_events,                
                UpdateEvent {                    
                    value : *& exchange_rate.value,
                    timestamp : exchange_rate.timestamp,
                    currency_code : Diem::currency_code<CoinType>()                    
                }
            );
    }

    fun administrator_address() : address {
        @0x4f524143
    }

    /// update exchange rate, if the exchange rate for CoinType doesn't exist then create it 
    public fun update_exchange_rate<CoinType: store>(
        admin_account : &signer, 
        numerator : u64, 
        denominator: u64
    ) acquires ExchangeRate {
        assert(
            Signer::address_of(admin_account) == administrator_address(), 
            EINVALID_SINGLETON_ADDRESS
        );        

        assert(Diem::is_currency<CoinType>(), ENOT_A_REGISTERED_CURRENCY);        

        if(!exists<ExchangeRate<CoinType>>(administrator_address())) {  
            let exchange_rate = ExchangeRate<CoinType> {
                value : FixedPoint32::create_from_rational(numerator, denominator), 
                timestamp : DiemTimestamp::now_microseconds(),
                update_events : Event::new_event_handle<UpdateEvent>(admin_account)
            };
        
            emit_updating_events(&mut exchange_rate);

            move_to(
                admin_account, 
                exchange_rate,
                );
        }
        else {
            let exchange_rate = borrow_global_mut<ExchangeRate<CoinType>>(administrator_address()); 

            exchange_rate.value = FixedPoint32::create_from_rational(numerator, denominator);
            exchange_rate.timestamp = DiemTimestamp::now_microseconds();

            emit_updating_events(exchange_rate);
        }
        
    }

    /// get exchange rate for CoinType
    public fun get_exchange_rate<CoinType: store>() : (FixedPoint32, u64) acquires ExchangeRate    
    {
        assert(Diem::is_currency<CoinType>(), ENOT_A_REGISTERED_CURRENCY);        

        let exchange_rate = borrow_global<ExchangeRate<CoinType>>(administrator_address()); //CoreAddresses::Diem_ROOT_ADDRESS()

        (*&exchange_rate.value, exchange_rate.timestamp)
    }
}


}
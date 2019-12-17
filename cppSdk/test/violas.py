try:
    import violas;

    client = violas.Client("52.151.2.66", 
                            40001,  
                            "consensus_peers.config.toml",
                            "temp_faucet_keys",
                            False,
                            "faucet.testnet.libra.org", 
                            "mnemonic")
    
    client.test_validator_connection()

    for i in range(5):    
        client.create_next_account(True)

    accounts = client.get_all_accounts()

    for a in accounts.items()
        print("", a.index)
        
    print("Finished test job !")

except BaseException as e:
    print(e)




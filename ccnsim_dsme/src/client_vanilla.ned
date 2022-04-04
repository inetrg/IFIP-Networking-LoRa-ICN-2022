package ccnsim_dsme;
import modules.clients.client;

simple client_vanilla extends client{
    int numInterestRetries;
    volatile double client_warmup @unit(s);
    bool client_deterministic = default(false);
	@class(client_vanilla);
}




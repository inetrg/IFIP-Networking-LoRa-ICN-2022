package ccnsim_dsme.simulations;
import networks.base_network;

network star_network extends base_network
{
    parameters:
        //Number of repositories
        // node_repos = "1,2,3,4,5,6,7,8,9";
        // num_repos = 9;
        // node_repos="2,4,6,8";
        node_repos=default("0");
        num_repos = default(1);
        replicas = 1;

        //Number of clients
        num_clients = default(1);
        node_clients = default("0");


    connections allowunconnected:
        for i = 0..n-2{
	        node[0].face++ <--> { delay = 0ms; } <--> node[i+1].face++;
	    }
        // node[0].face++ <--> {delay = 1ms;} <--> node[1].face++;
        // node[0].face++ <--> {delay = 1ms;} <--> node[2].face++;
        // node[0].face++ <--> {delay = 1ms;} <--> node[3].face++;
        // node[0].face++ <--> {delay = 1ms;} <--> node[4].face++;
        // node[0].face++ <--> {delay = 1ms;} <--> node[5].face++;
        // node[0].face++ <--> {delay = 1ms;} <--> node[6].face++;
        // node[0].face++ <--> {delay = 1ms;} <--> node[7].face++;
        // node[0].face++ <--> {delay = 1ms;} <--> node[8].face++;
        // node[0].face++ <--> {delay = 1ms;} <--> node[9].face++;
}

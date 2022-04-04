#include "content_distribution.h"

using namespace std;

class content_distribution_ext : public content_distribution{
    public:
		void init_content();
};

Register_Class(content_distribution_ext);

void content_distribution_ext::init_content()
{
    // In repo_card we count how many objects each repo is storing.
	vector<int> repo_card(num_repos,0); 

    // As the repositories are represented as a string of bits, the function
    // binary_string is used for generating binary strings of length num_repos
    // with exactly replicas ones, where "replicas" is the number of replicas for each object.
	// Each string represents a replica placement of a certain object among the repositories.
	// Given a single string, a 1 in the i-th position means that a replica of that object
	// is placed in the i-th repository.
    // repo_strings = binary_strings(replicas, num_repos);

	EV << "replicas :" << replicas << "num_repos: "<< num_repos<<"\n";
	//EV << "repo_strings.size(): " << repo_strings.size() <<"\n";
	//EV << "sizeof(repo_strings): " << sizeof(repo_strings) << "\n";

	//for (int d = 1; d <= cardF; d++)
    for (int d = 1; d <= newCardF; d++) // pk: for every unique content item (catalog size) starting from one
    {
    	// 'd' is a content.
		// Reset the information field of a given content
		__info(d) = 0;

		// we ignore replicas with this
		int to_repo=repositories[d % num_repos];
		content_distribution::catalog[d].cont_repos.push_back(to_repo);
	}

	// print catalog
	for (int i = 0; i < content_distribution::catalog.size(); i++)
	{
		EV << "Content #"<< i<< "; info value: "<< content_distribution::catalog[i].info<<endl;
		for (int j = 0; j < content_distribution::catalog[i].cont_repos.size(); j++)
		{
			EV << "  Repo #: " << content_distribution::catalog[i].cont_repos[j] << endl;
		}
	}

	// Record the repository cardinality and price
	for (int repo_idx = 0; repo_idx < num_repos; repo_idx++){
	    char name[15];
		EV<<"repo_card["<<repo_idx<<"]: "<<repo_card[repo_idx]<<"\n";

		sprintf(name,"repo-%d_card",repo_idx);
		recordScalar(name, repo_card[repo_idx] );

		sprintf(name,"repo-%d_price",repo_idx);
		recordScalar(name, repo_prices[repo_idx] ); 

	}
}

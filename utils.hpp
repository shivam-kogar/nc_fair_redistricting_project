#pragma once
#include <string>
#include <vector>
#include <unordered_map>
#include <algorithm> 
#include <cctype>    
#include <fstream>   // for std::ifstream, std::ofstream
#include <sstream>   // for std::stringstream (if you're parsing CSV)
#include <string>    // for std::string


using namespace std;

using Neighbor = string;
using Graph = unordered_map<string, vector<Neighbor>>;

string normalize(const string& s) {
    string t = s;

    while (!t.empty() && isspace(t.front())) t.erase(t.begin());
    while (!t.empty() && isspace(t.back())) t.pop_back();

    t.erase(remove_if(t.begin(), t.end(), ::isspace), t.end());

    return t;
}

vector<vector<string>> read_csv(const string& filename); 

Graph build_graph(const string& filename) {
    auto csv = read_csv(filename);
    Graph g;

    for (size_t i = 1; i < csv.size(); ++i) { 
        string a = normalize(csv[i][0]);
        string b = normalize(csv[i][1]);

        g[a].push_back(b);
        g[b].push_back(a);
    }
    return g;
}

void remove_additional_edges(Graph& g) {
    for (auto& [node, neighbors] : g) {
        sort(neighbors.begin(), neighbors.end());
        neighbors.erase(unique(neighbors.begin(), neighbors.end()), neighbors.end());
    }
}



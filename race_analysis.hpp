
#include <string>
#include <unordered_map>
#include "utils.hpp"

using namespace std;

using RaceData = unordered_map<string, Demographics>;

struct Demographics {
    double hispanic;
    double black;
    double white;
    double asian;
    double american_indian;
};

RaceData load_race_data(const string& filename, Demographics
& stdev);
double race_distance(const Demographics
& a, const Demographics
& b, const Demographics
& stdev);
vector<vector<string>> read_csv(const string& filename);
double zdist(double a, double b, double stdev);
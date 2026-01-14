#include <iostream>
#include <fstream>
#include <sstream>
#include <unordered_map>
#include <unordered_set>
#include <vector>
#include <queue>
#include <algorithm>
#include <cmath>
#include <cassert>
#include <filesystem>
#include <random>
#include <ctime>

using namespace std;
namespace fs = std::filesystem;

vector<vector<string>> read_csv(const string& filename);


struct Demographics {
    double hispanic = 0;
    double black = 0;
    double white = 0;
    double asian = 0;
    double american_indian = 0;
};

struct County {
    string name;
    double population = 0;

    Demographics race;
    double food_count = 0;      // population receiving SNAP
    double wage_score_sum = 0;  // normalized_wage_score * population
};

struct District {
    vector<string> members;
    double population_sum = 0;

    Demographics race_sum;
    double food_sum = 0;
    double wage_score_sum = 0;

    // base counties present in this district 
    unordered_set<string> bases;
};

struct DistrictProfile {
    Demographics race;
    double food_score = 0;
    double wage_score = 0;
};

struct DistanceWeights {
    Demographics race_stdev{};
    double food_stdev = 1.0;
    double wages_stdev = 1.0;

    double w_race = 1.0;
    double w_food = 1.0;
    double w_wages = 1.0;
};

double zdist(double a, double b, double stdev) {
    const double eps = 1e-9;
    return fabs(a - b) / (stdev + eps);
}

DistrictProfile compute_profile(const District& D) {
    assert(D.population_sum > 0);
    double P = D.population_sum;

    DistrictProfile p;
    p.race.hispanic        = D.race_sum.hispanic        / P;
    p.race.black           = D.race_sum.black           / P;
    p.race.white           = D.race_sum.white           / P;
    p.race.asian           = D.race_sum.asian           / P;
    p.race.american_indian = D.race_sum.american_indian / P;

    p.food_score = D.food_sum       / P;
    p.wage_score = D.wage_score_sum / P;

    return p;
}

DistrictProfile county_profile(const County& C) {
    assert(C.population > 0);
    DistrictProfile p;
    // race fields are raw counts in County; we want proportions in Profile
    p.race.hispanic        = C.race.hispanic        / C.population;
    p.race.black           = C.race.black           / C.population;
    p.race.white           = C.race.white           / C.population;
    p.race.asian           = C.race.asian           / C.population;
    p.race.american_indian = C.race.american_indian / C.population;

    p.food_score = C.food_count      / C.population;
    p.wage_score = C.wage_score_sum  / C.population;
    return p;
}

double distance(const DistrictProfile& A,
                const DistrictProfile& B,
                const DistanceWeights& W) {

    double race_dist =
          zdist(A.race.hispanic,        B.race.hispanic,        W.race_stdev.hispanic)
        + zdist(A.race.black,           B.race.black,           W.race_stdev.black)
        + zdist(A.race.white,           B.race.white,           W.race_stdev.white)
        + zdist(A.race.asian,           B.race.asian,           W.race_stdev.asian)
        + zdist(A.race.american_indian, B.race.american_indian, W.race_stdev.american_indian);

    double f = zdist(A.food_score, B.food_score, W.food_stdev);
    double w = zdist(A.wage_score, B.wage_score, W.wages_stdev);

    return W.w_race * race_dist +
           W.w_food * f +
           W.w_wages * w;
}


using Graph = unordered_map<string, vector<string>>;

string normalize(string s) {
    // trim
    while (!s.empty() && isspace(s.back())) s.pop_back();
    while (!s.empty() && isspace(s.front())) s.erase(s.begin());

    // lowercase
    transform(s.begin(), s.end(), s.begin(), ::tolower);

    // remove commas
    s.erase(remove(s.begin(), s.end(), ','), s.end());

    // remove spaces entirely
    s.erase(remove_if(s.begin(), s.end(), ::isspace), s.end());

    // remove "county" suffix
    const string suffix = "county";
    if (s.size() >= suffix.size() &&
        s.compare(s.size() - suffix.size(), suffix.size(), suffix) == 0) {
        s.erase(s.size() - suffix.size());
    }

    return s;
}

// base county name, remove additional syntax
string base_name(const string& s) {
    auto pos = s.find('#');
    if (pos == string::npos) return s;
    return s.substr(0, pos);
}

Graph build_graph(const string& filename) {
    Graph g;
    auto csv = read_csv(filename);

    for (size_t i = 1; i < csv.size(); ++i) {
        string a = normalize(csv[i][0]);
        string b = normalize(csv[i][1]);
        g[a].push_back(b);
        g[b].push_back(a);
    }
    return g;
}

void dedup_graph(Graph& g) {
    for (auto& kv : g) {
        auto& neighbors = kv.second;
        sort(neighbors.begin(), neighbors.end());
        neighbors.erase(unique(neighbors.begin(), neighbors.end()), neighbors.end());
    }
}

unordered_map<string, County> load_counties(const string& race_csv,
                                            const string& food_csv,
                                            const string& wage_csv) {
    unordered_map<string, County> counties;

    // load race data (raw counts + Total_Pop)
    auto race = read_csv(race_csv);
    for (size_t i = 1; i < race.size(); ++i) {
        County C;
        C.name       = normalize(race[i][0]);
        C.population = stod(race[i][1]);

        C.race.hispanic        = stod(race[i][2]); // Hispanic of any race
        // race[i][3] = Non-Hispanic, ignored
        C.race.white           = stod(race[i][4]); // White
        C.race.black           = stod(race[i][5]); // Black
        C.race.american_indian = stod(race[i][6]); // AI/AN
        C.race.asian           = stod(race[i][7]); // Asian

        counties[C.name] = C;
    }

    auto food = read_csv(food_csv);
    for (size_t i = 1; i < food.size(); ++i) {
        string name = normalize(food[i][0]);
        if (!counties.count(name)) continue;

        double pct = stod(food[i][2]); // decimal format
        counties[name].food_count = pct * counties[name].population;
    }

    auto wage = read_csv(wage_csv);
    for (size_t i = 1; i < wage.size(); ++i) {
        string name = normalize(wage[i][0]);
        if (!counties.count(name)) continue;

        double score = stod(wage[i][2]); // normalized wage score
        counties[name].wage_score_sum = score * counties[name].population;
    }

    return counties;
}

struct Candidate {
    double dist;
    int d_id;
    string county;
};

struct Cmp {
    bool operator()(const Candidate& a, const Candidate& b) const {
        return a.dist > b.dist;
    }
};

void add_to_district(District& D, const County& C) {
    D.members.push_back(C.name);
    D.population_sum += C.population;

    D.race_sum.hispanic        += C.race.hispanic;
    D.race_sum.black           += C.race.black;
    D.race_sum.white           += C.race.white;
    D.race_sum.asian           += C.race.asian;
    D.race_sum.american_indian += C.race.american_indian;

    D.food_sum       += C.food_count;
    D.wage_score_sum += C.wage_score_sum;

    D.bases.insert(base_name(C.name));
}

vector<string> choose_seeds(
    const Graph& g,
    const unordered_map<string, County>& counties,
    const DistanceWeights& W,
    int k)
{
    (void)g; // currently unused, but left in signature for future geographic logic

    vector<string> names;
    names.reserve(counties.size());
    for (const auto& kv : counties) {
        names.push_back(kv.first);
    }

    // shuffle for randomness between runs
    std::mt19937 rng(static_cast<unsigned>(time(NULL)));
    std::shuffle(names.begin(), names.end(), rng);

    vector<string> seeds;
    vector<double> minDist(names.size(), 1e18);
    unordered_set<string> used_bases;

    // pick first seed
    size_t first_valid = 0;
    if (first_valid == names.size()) {
        throw runtime_error("ERROR: no valid seed found!");
    }
    seeds.push_back(names[first_valid]);
    used_bases.insert(base_name(names[first_valid]));

    // pick remaining seeds, far apart in feature space & different base counties
    while (seeds.size() < static_cast<size_t>(k)) {
        string last = seeds.back();
        DistrictProfile lastP = county_profile(counties.at(last));

        for (size_t i = 0; i < names.size(); ++i) {
            DistrictProfile p = county_profile(counties.at(names[i]));
            double d = distance(lastP, p, W);
            minDist[i] = min(minDist[i], d);
        }

        size_t best = names.size();
        for (size_t i = 0; i < names.size(); ++i) {
            string b = base_name(names[i]);
            if (used_bases.count(b)) continue; // don't pick another part of same county
            if (best == names.size() || minDist[i] > minDist[best]) {
                best = i;
            }
        }

        if (best == names.size()) {
            // ran out of distinct base counties; stop early
            break;
        }

        seeds.push_back(names[best]);
        used_bases.insert(base_name(names[best]));
    }

    return seeds;
}

vector<District> cluster(
    const Graph& g,
    unordered_map<string, County>& counties,
    const DistanceWeights& W,
    int num_districts = 14)
{
    vector<string> seeds = choose_seeds(g, counties, W, num_districts);

    vector<District> D(num_districts);
    unordered_map<string, int> assigned;

    double total_pop = 0;
    for (const auto& kv : counties) total_pop += kv.second.population;
    double target = total_pop / num_districts;
    double tol = 0.05; // 5%

    // initialize districts with seeds, but forbid seeds that are already illegal
    for (int i = 0; i < num_districts && i < static_cast<int>(seeds.size()); ++i) {
        const County& C = counties.at(seeds[i]);
        if (C.population > target * (1.0 + tol)) {
            // seed is too large to ever be valid
            cerr << "Seed county too large for district: " << C.name
                 << " pop=" << C.population << "\n";
            return {};
        }
        add_to_district(D[i], C);
        assigned[seeds[i]] = i;
    }

    priority_queue<Candidate, vector<Candidate>, Cmp> pq;

    auto push_neighbors = [&](int d_id) {
        auto profile = compute_profile(D[d_id]);
        for (const auto& cty : D[d_id].members) {
            if (!g.count(cty)) {
                cerr << "ERROR: county in district not found in graph: [" << cty << "]\n";
                continue;
            }
            for (const auto& nbr : g.at(cty)) {
                if (assigned.count(nbr)) continue;

                auto dist = distance(profile, county_profile(counties.at(nbr)), W);
                pq.push({dist, d_id, nbr});
            }
        }
    };

    for (int i = 0; i < num_districts && i < static_cast<int>(seeds.size()); ++i) {
        push_neighbors(i);
    }

    while (assigned.size() < counties.size()) {
        if (pq.empty()) {
            cerr << "ERROR: PQ exhausted but counties remain\n";
            return {};
        }

        auto c = pq.top();
        pq.pop();

        if (assigned.count(c.county)) continue;

        const County& C = counties.at(c.county);
        District& d = D[c.d_id];

        string base = base_name(C.name);
        // forbid multiple parts of same base county in one district
        if (d.bases.count(base)) {
            continue;
        }

        // population constraint
        if (d.population_sum + C.population > target * (1.0 + tol)) {
            continue;
        }

        assigned[C.name] = c.d_id;
        add_to_district(d, C);
        push_neighbors(c.d_id);
    }

    // final sanity check: no district above tolerance
    for (const auto& d : D) {
        if (d.population_sum > target * (1.0 + tol)) {
            cerr << "District too large: " << d.population_sum << "\n";
            return {};
        }
    }

    return D;
}

// ======== CSV Reader ==============================================

vector<vector<string>> read_csv(const string& filename) {
    ifstream file(filename);
    string line;
    vector<vector<string>> data;

    while (getline(file, line)) {
        stringstream ss(line);
        string cell;
        vector<string> row;

        while (getline(ss, cell, ',')) {
            row.push_back(cell);
        }

        if (!row.empty()) data.push_back(row);
    }
    return data;
}

// ======== Plan Saver ==============================================

void save_plan(const vector<District>& D,
               const unordered_map<string, County>& counties,
               int id) {
    fs::create_directory("plans");

    ofstream out("plans/plan_" + to_string(id) + ".csv");

    // District totals
    out << "district,total_population\n";
    for (int i = 0; i < static_cast<int>(D.size()); ++i) {
        out << i << "," << static_cast<long long>(D[i].population_sum) << "\n";
    }

    out << "\n";

    // County assignment
    out << "district,county,county_population\n";
    for (int i = 0; i < static_cast<int>(D.size()); ++i) {
        for (const auto& c : D[i].members) {
            auto it = counties.find(c);
            double pop = (it != counties.end()) ? it->second.population : 0.0;
            out << i << "," << c << "," << static_cast<long long>(pop) << "\n";
        }
    }
}

// ======== MAIN ====================================================

int main() {
    srand(static_cast<unsigned>(time(NULL)));

    Graph g = build_graph("data/county_edge_list.csv");
    dedup_graph(g);

    auto counties = load_counties(
        "data/race_data.csv",
        "data/food_data.csv",
        "data/wage_data.csv"
    );

    DistanceWeights W;
    W.w_race  = 0.33;
    W.w_food  = 0.33;
    W.w_wages = 0.33;

    const int NUM_DISTRICTS = 14;
    const int MAX_ATTEMPTS  = 2000;
    const int TARGET_PLANS  = 50;

    int saved = 0;

    for (int attempt = 1; attempt <= MAX_ATTEMPTS && saved < TARGET_PLANS; ++attempt) {
        auto districts = cluster(g, counties, W, NUM_DISTRICTS);

        if (districts.empty()) {
            continue; // invalid plan
        }

        bool valid = true;
        for (const auto& D : districts) {
            if (D.population_sum <= 0) {
                valid = false;
                break;
            }
        }
        if (!valid) continue;

        save_plan(districts, counties, saved);
        ++saved;

        cerr << "✔ Saved plan " << saved << " (attempt " << attempt << ")\n";
    }

    if (saved == 0) {
        cerr << "FATAL: No valid plans generated.\n";
        return 1;
    }

    cout << "=== DONE ===\nGenerated " << saved << " valid plans.\n";
    return 0;
}

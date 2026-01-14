#include "race_analysis.hpp"
#include "utils.hpp"

RaceData load_race_data(const string& filename, Demographics& stdev) {
    auto csv = read_csv(filename);
    RaceData county_race_data;

    for (size_t i = 1; i < csv.size(); ++i) {
        if (csv[i][0] == "STDEV") {
            // Column order:
            // STDEV, Total_Pop (ignore), Hispanic, NonHispanic(ignore), White, Black, AI/AN, Asian
            stdev = {
                stod(csv[i][2]), // Hispanic
                stod(csv[i][5]), // Black
                stod(csv[i][4]), // White
                stod(csv[i][7]), // Asian
                stod(csv[i][6])  // AI/AN
            };
        } else {
            string name = csv[i][0];
            county_race_data[name] = {
                stod(csv[i][2]), // Hispanic
                stod(csv[i][5]), // Black
                stod(csv[i][4]), // White
                stod(csv[i][7]), // Asian
                stod(csv[i][6])  // AI/AN
            };
        }
    }
    return county_race_data;
}

double race_distance(const Demographics& A,
                     const Demographics& B,
                     const Demographics& stdev) {

    return zdist(A.hispanic,        B.hispanic,        stdev.hispanic) +
           zdist(A.black,           B.black,           stdev.black) +
           zdist(A.white,           B.white,           stdev.white) +
           zdist(A.asian,           B.asian,           stdev.asian) +
           zdist(A.american_indian, B.american_indian, stdev.american_indian);
}

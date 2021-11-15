#include <random>
#include <vector>

#include "FastMIDyNet/generators.h"
#include "FastMIDyNet/types.h"


using namespace std;
namespace FastMIDyNet {
int generateCategorical(vector<double> probs, RNG rng){
    discrete_distribution<double> dist(probs.begin(), probs.end());
    return dist(rng);
}
}

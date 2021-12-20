#ifndef FAST_MIDYNET_RNG_H
#define FAST_MIDYNET_RNG_H


#include <random>
#include "FastMIDyNet/types.h"


namespace FastMIDyNet {

extern RNG rng;

void seed(size_t n);
void seedWithTime();

} // namespace FastMIDyNet

#endif

#include "DistribFlow.h"

using namespace stargate;

namespace {

bool _infraYesMode = false;

}

void stargate::setInfraYesMode(bool yes) {
    _infraYesMode = yes;
}

bool stargate::isInfraYesMode() {
    return _infraYesMode;
}

DistribFlow::DistribFlow() {
}

DistribFlow::~DistribFlow() {
}

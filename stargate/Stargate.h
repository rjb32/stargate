#pragma once

namespace stargate {

class StargateConfig;

class Stargate {
public:
    Stargate(const StargateConfig& config);
    ~Stargate();

    void run();

private:
    const StargateConfig& _config;

    void createOutputDir();
};

}
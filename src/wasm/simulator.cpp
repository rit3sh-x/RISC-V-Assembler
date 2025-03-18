#include <emscripten/bind.h>
#include <emscripten/val.h>
#include "simulator.hpp"

using namespace emscripten;

val mapToVal(const std::map<uint32_t, std::pair<uint32_t, std::string>>& m) {
    val result = val::object();
    for (const auto& [key, value] : m) {
        val pair = val::object();
        pair.set("first", value.first);
        pair.set("second", value.second);
        result.set(std::to_string(key), pair);
    }
    return result;
}

val mapUint32ToVal(const std::map<uint32_t, uint32_t>& m) {
    val result = val::object();
    for (const auto& [key, value] : m) {
        result.set(std::to_string(key), value);
    }
    return result;
}

val mapIntToStringVal(const std::map<int, std::string>& m) {
    val result = val::object();
    for (const auto& [key, value] : m) {
        result.set(std::to_string(key), value);
    }
    return result;
}

val unorderedMapToVal(const std::unordered_map<uint32_t, uint8_t>& m) {
    val result = val::object();
    for (const auto& [key, value] : m) {
        result.set(std::to_string(key), value);
    }
    return result;
}

class SimulatorWrapper {
public:
    SimulatorWrapper() : sim(), lastError("") {}
    
    bool loadProgram(const std::string& input) { 
        try {
            return sim.loadProgram(input);
        } catch (const std::exception& e) {
            sim.logs[404] = "Error loading program";
            return false;
        }
    }
    
    bool step() { 
        try {
            return sim.step();
        } catch (const std::exception& e) {
            sim.logs[404] = "An Exception occured";
            return false;
        }
    }
    
    void run() { 
        try {
            sim.run();
        } catch (const std::exception& e) {
            sim.logs[404] = "An Exception occured";
        }
    }

    void reset(){
        try {
            sim.reset();
        } catch (const std::exception& e) {
            sim.logs[404] = "An Exception occured";
        }
    }

    val getRegisters() {
        const uint32_t* regs = sim.getRegisters();
        val result = val::array();
        for (int i = 0; i < 32; i++) {
            result.set(i, regs[i]);
        }
        return result;
    }
    
    uint32_t getPC() const { return sim.getPC(); }
    uint32_t getCycles() const { return sim.getCycles(); }
    val getDataMap() const { return unorderedMapToVal(sim.getDataMap()); }
    val getTextMap() const { return mapToVal(sim.getTextMap()); }
    val getMemoryChanges() const { return mapUint32ToVal(sim.getMemoryChanges()); }
    val getConsoleOutput() const { return mapIntToStringVal(sim.getConsoleOutput()); }
    int getCurrentStage() const { return static_cast<int>(sim.getCurrentStage()); }
    bool isRunning() const { return sim.isRunning(); }
    val getPipelineRegisters() {
        try {
            const auto& regs = sim.getPipelineRegisters();
            val result = val::object();
            result.set("RA", regs.RA);
            result.set("RB", regs.RB);
            result.set("RM", regs.RM);
            result.set("RY", regs.RY);
            result.set("RZ", regs.RZ);
            
            return result;
        } catch (const std::exception& e) {
            sim.logs[400] = "Unknown error";
            return val::object();
        }
    }

private:
    Simulator sim;
    std::string lastError;
};

EMSCRIPTEN_BINDINGS(simulator_module) {
    enum_<Stage>("Stage")
        .value("FETCH", Stage::FETCH)
        .value("DECODE", Stage::DECODE)
        .value("EXECUTE", Stage::EXECUTE)
        .value("MEMORY", Stage::MEMORY)
        .value("WRITEBACK", Stage::WRITEBACK);
        
    class_<SimulatorWrapper>("Simulator")
        .constructor<>()
        .function("loadProgram", &SimulatorWrapper::loadProgram)
        .function("step", &SimulatorWrapper::step)
        .function("run", &SimulatorWrapper::run)
        .function("reset", &SimulatorWrapper::reset)
        .function("getRegisters", &SimulatorWrapper::getRegisters)
        .function("getPC", &SimulatorWrapper::getPC)
        .function("getCycles", &SimulatorWrapper::getCycles)
        .function("getDataMap", &SimulatorWrapper::getDataMap)
        .function("getTextMap", &SimulatorWrapper::getTextMap)
        .function("getConsoleOutput", &SimulatorWrapper::getConsoleOutput)
        .function("getCurrentStage", &SimulatorWrapper::getCurrentStage)
        .function("isRunning", &SimulatorWrapper::isRunning)
        .function("getPipelineRegisters", &SimulatorWrapper::getPipelineRegisters);
}
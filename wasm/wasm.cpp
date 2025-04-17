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

val mapStageToVal(const std::map<Stage, bool>& m) {
    val result = val::object();
    for (const auto& [key, value] : m) {
        std::string stageName;
        switch (key) {
            case Stage::FETCH: stageName = "FETCH"; break;
            case Stage::DECODE: stageName = "DECODE"; break;
            case Stage::EXECUTE: stageName = "EXECUTE"; break;
            case Stage::MEMORY: stageName = "MEMORY"; break;
            case Stage::WRITEBACK: stageName = "WRITEBACK"; break;
        }
        result.set(stageName, value);
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

val unorderedMapIntStringToVal(const std::unordered_map<int, std::string>& m) {
    val result = val::object();
    for (const auto& [key, value] : m) {
        result.set(std::to_string(key), value);
    }
    return result;
}

val instructionRegistersToVal(const InstructionRegisters& regs) {
    val result = val::object();
    result.set("RA", regs.RA);
    result.set("RB", regs.RB);
    result.set("RM", regs.RM);
    result.set("RY", regs.RY);
    result.set("RZ", regs.RZ);
    return result;
}

val uiResponseToVal(const UIResponse& response) {
    val result = val::object();
    result.set("isFlushed", response.isFlushed);
    result.set("isStalled", response.isStalled);
    result.set("isDataForwarded", response.isDataForwarded);
    result.set("isProgramTerminated", response.isProgramTerminated);
    return result;
}

class SimulatorWrapper {
public:
    SimulatorWrapper() : sim() {}
    
    bool loadProgram(const std::string& input) { 
        try {
            return sim.loadProgram(input);
        } catch (const std::exception& e) {
            logs[404] = "Error loading program: " + std::string(e.what());
            return false;
        }
    }
    
    bool step() { 
        try {
            return sim.step();
        } catch (const std::exception& e) {
            logs[404] = "Exception during execution step: " + std::string(e.what());
            return false;
        }
    }
    
    void run() { 
        try {
            sim.run();
        } catch (const std::exception& e) {
            logs[404] = "Exception during full program execution: " + std::string(e.what());
        }
    }

    void reset() {
        try {
            sim.reset();
        } catch (const std::exception& e) {
            logs[404] = "Exception during simulator reset: " + std::string(e.what());
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
    val getLogs() { return unorderedMapIntStringToVal(sim.getLogs()); }
    bool isRunning() const { return sim.isRunning(); }
    
    val getActiveStages() const { 
        std::map<Stage, std::pair<bool, uint32_t>> stages = sim.getActiveStages();
        val result = val::object();
        for (const auto& [stage, data] : stages) {
            val stageData = val::object();
            stageData.set("active", data.first);
            stageData.set("instruction", data.second);
            result.set(static_cast<int>(stage), stageData);
        }
        return result;
    }
    
    uint32_t getStalls() const { return sim.getStalls(); }
    void setEnvironment(bool pipeline, bool dataForwarding) {
        sim.setEnvironment(pipeline, dataForwarding);
    }

    val getUIResponse() const {
        return uiResponseToVal(sim.getUIResponse());
    }

    val getInstructionRegisters() const {
        return instructionRegistersToVal(sim.getInstructionRegisters());
    }

private:
    Simulator sim;
    std::unordered_map<int, std::string> logs;
};

EMSCRIPTEN_BINDINGS(simulator_module) {
    enum_<Stage>("Stage")
        .value("FETCH", Stage::FETCH)
        .value("DECODE", Stage::DECODE)
        .value("EXECUTE", Stage::EXECUTE)
        .value("MEMORY", Stage::MEMORY)
        .value("WRITEBACK", Stage::WRITEBACK);

    value_object<InstructionRegisters>("InstructionRegisters")
        .field("RA", &InstructionRegisters::RA)
        .field("RB", &InstructionRegisters::RB)
        .field("RM", &InstructionRegisters::RM)
        .field("RY", &InstructionRegisters::RY)
        .field("RZ", &InstructionRegisters::RZ);

    value_object<UIResponse>("UIResponse")
        .field("isFlushed", &UIResponse::isFlushed)
        .field("isStalled", &UIResponse::isStalled)
        .field("isDataForwarded", &UIResponse::isDataForwarded)
        .field("isProgramTerminated", &UIResponse::isProgramTerminated);
        
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
        .function("getLogs", &SimulatorWrapper::getLogs)
        .function("isRunning", &SimulatorWrapper::isRunning)
        .function("getActiveStages", &SimulatorWrapper::getActiveStages)
        .function("getStalls", &SimulatorWrapper::getStalls)
        .function("setEnvironment", &SimulatorWrapper::setEnvironment)
        .function("getInstructionRegisters", &SimulatorWrapper::getInstructionRegisters)
        .function("getUIResponse", &SimulatorWrapper::getUIResponse);
}
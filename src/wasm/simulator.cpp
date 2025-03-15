#include <emscripten/bind.h>
#include "simulator.hpp"

using namespace emscripten;

class SimulatorWrapper : public Simulator {
public:
    SimulatorWrapper() : Simulator() {}

    bool loadProgramJS(const std::string& program) {
        return loadProgram(program);
    }

    bool stepJS() {
        return step();
    }

    void runJS() {
        run();
    }

    std::string parseInstructionsJS(uint32_t instHex) {
        return parseInstructions(instHex);
    }

    val getRegistersJS() const {
        const uint32_t* regs = getRegisters();
        std::vector<uint32_t> regVec(regs, regs + 32);
        return val(regVec);
    }

    uint32_t getPCJS() const {
        return getPC();
    }

    uint32_t getCyclesJS() const {
        return getCycles();
    }

    val getDataMapJS() const {
        std::map<uint32_t, uint8_t> orderedMap(getDataMap().begin(), getDataMap().end());
        return val(orderedMap);
    }
    
    val getTextMapJS() const {
        std::map<uint32_t, std::pair<uint32_t, std::string>> text = getTextMap();
        val result = val::object();
        for (const auto& [addr, pair] : text) {
            val pairObj = val::object();
            pairObj.set("first", pair.first);
            pairObj.set("second", pair.second);
            result.set(addr, pairObj);
        }
        return result;
    }
    
    val getMemoryChangesJS() const {
        return val(getMemoryChanges());
    }

    Stage getCurrentStageJS() const {
        return getCurrentStage();
    }
    
    val getConsoleOutputJS() const {
        std::map<std::string, std::string> output = getConsoleOutput();
        val result = val::object();
        for (const auto& [key, value] : output) {
            result.set(key, value);
        }
        return result;
    }

    bool isRunningJS() const {
        return isRunning();
    }
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
        .function("loadProgram", &SimulatorWrapper::loadProgramJS)
        .function("step", &SimulatorWrapper::stepJS)
        .function("run", &SimulatorWrapper::runJS)
        .function("parseInstructions", &SimulatorWrapper::parseInstructionsJS)
        .function("getRegisters", &SimulatorWrapper::getRegistersJS)
        .function("getPC", &SimulatorWrapper::getPCJS)
        .function("getCycles", &SimulatorWrapper::getCyclesJS)
        .function("getDataMap", &SimulatorWrapper::getDataMapJS)
        .function("getTextMap", &SimulatorWrapper::getTextMapJS)
        .function("getMemoryChanges", &SimulatorWrapper::getMemoryChangesJS)
        .function("getCurrentStage", &SimulatorWrapper::getCurrentStageJS)
        .function("getConsoleOutput", &SimulatorWrapper::getConsoleOutputJS)
        .function("isRunning", &SimulatorWrapper::isRunningJS);

    register_vector<uint32_t>("vector<uint32_t>");
    register_map<uint32_t, uint8_t>("map<uint32_t, uint8_t>");
    register_map<uint32_t, uint32_t>("map<uint32_t, uint32_t>");
    register_map<std::string, std::string>("map<string, string>");
}
#include <emscripten/bind.h>
#include "simulator.hpp"

using namespace emscripten;

class SimulatorWrapper : public Simulator {
public:
    SimulatorWrapper() : Simulator() {}

    val getRegistersJS() const {
        const uint32_t* regs = getRegisters();
        std::vector<uint32_t> regVec(regs, regs + 32);
        return val(regVec);
    }

    val getDataMapJS() const {
        return val(getDataMap());
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
    
    val getConsoleOutputJS() {
        return val(getConsoleOutput());
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
        .function("loadProgram", &SimulatorWrapper::loadProgram)
        .function("step", &SimulatorWrapper::step)
        .function("run", &SimulatorWrapper::run)
        .function("parseInstructions", &SimulatorWrapper::parseInstructions)
        .function("getRegisters", &SimulatorWrapper::getRegistersJS)
        .function("getPC", &SimulatorWrapper::getPC)
        .function("getCycles", &SimulatorWrapper::getCycles)
        .function("getDataMap", &SimulatorWrapper::getDataMapJS)
        .function("getTextMap", &SimulatorWrapper::getTextMapJS)
        .function("getMemoryChanges", &SimulatorWrapper::getMemoryChangesJS)
        .function("getCurrentStage", &SimulatorWrapper::getCurrentStage)
        .function("getConsoleOutput", &SimulatorWrapper::getConsoleOutputJS)
        .function("isRunning", &SimulatorWrapper::isRunning);

    register_vector<uint32_t>("vector<uint32_t>");
    register_map<uint32_t, uint8_t>("map<uint32_t, uint8_t>");
    register_map<uint32_t, uint32_t>("map<uint32_t, uint32_t>");
    register_map<std::string, std::string>("map<string, string>");
}

int main() {
    return 0;
}